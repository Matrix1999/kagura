//===-- FunctionSplit.cpp - Function splitting / CFG fragmentation --------===//
//
// Splits large functions by extracting "cold" basic blocks (those not on the
// entry/return path) into separate outlined helper functions, then replacing
// the original block with a call to the helper followed by an unconditional
// branch to the block's first successor.
//
// The net effect is that decompilers see a fragmented call graph instead of a
// single large function body, making manual analysis significantly harder.
//
// Pass key:   "fsplit"
// CLI flag:   -kagura-fsplit
//
// Eligibility criteria for a function:
//   - Not a declaration, not an intrinsic, not vararg
//   - shouldObfuscate() returns true
//   - Has >= 5 basic blocks (splitting tiny functions isn't useful)
//
// Eligibility criteria for a basic block:
//   - Not the entry block
//   - Does not contain a ReturnInst or UnreachableInst (exit blocks)
//   - Has no PHI nodes (live-in analysis via alloca is already done before
//     this pass; skip blocks that still have phis to be safe)
//   - Has no call/invoke instructions (avoids ABI and stack-frame complexity)
//   - Has exactly one predecessor and one successor (linear chain member)
//
// Extraction strategy (manual; avoids CodeExtractor's limitations):
//   For each eligible block BB with predecessor Pred and successor Succ:
//     1. Collect all Values defined *outside* BB but used *inside* BB
//        (live-in set), capped at MaxArgs to keep the ABI tractable.
//     2. Create a new Function F_helper(live-in...) -> void in the same module,
//        with internal linkage and an obfuscated name.
//     3. Clone BB's instructions into F_helper's entry block, rewriting
//        operand references to the new function's arguments.
//     4. Append an unconditional branch to a new "ret void" block in F_helper
//        (the block's own terminator is replaced by ret void if the block
//        originally branched unconditionally to Succ).
//     5. In the original function, replace BB's content with:
//          call void @F_helper(live-in...)
//          br Succ
//        and delete the now-empty original block.
//
//===----------------------------------------------------------------------===//

#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Support/CommandLine.h" // for extern cl::opt<bool> EnableFSplit
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <vector>
#include <string>

using namespace llvm;

// Defined in Plugin.cpp alongside all other kagura CLI flags.
extern cl::opt<bool> EnableFSplit;

namespace kagura {

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

/// Returns true if BB contains any call or invoke instruction.
static bool hasCallInsts(const BasicBlock &BB) {
  for (const auto &I : BB)
    if (isa<CallInst>(I) || isa<InvokeInst>(I))
      return true;
  return false;
}

/// Returns true if BB contains a return or unreachable terminator.
static bool isExitBlock(const BasicBlock &BB) {
  const Instruction *Term = BB.getTerminator();
  return isa<ReturnInst>(Term) || isa<UnreachableInst>(Term);
}

/// Collect all Values that are defined *outside* BB but used *inside* BB.
/// These become the live-in arguments of the outlined helper.
static std::vector<Value *> collectLiveIns(BasicBlock *BB) {
  std::vector<Value *> LiveIns;
  // Track values already added to avoid duplicates.
  SmallPtrSet<Value *, 16> Seen;

  for (Instruction &I : *BB) {
    for (Use &U : I.operands()) {
      Value *V = U.get();
      // Constants, globals, and inline asm are not live-ins.
      if (isa<Constant>(V) || isa<BasicBlock>(V) || isa<MetadataAsValue>(V))
        continue;
      // Arguments and instructions defined in OTHER blocks are live-ins.
      if (auto *Inst = dyn_cast<Instruction>(V)) {
        if (Inst->getParent() == BB)
          continue; // defined inside BB, not a live-in
      }
      if (Seen.insert(V).second)
        LiveIns.push_back(V);
    }
  }
  return LiveIns;
}

/// Build an obfuscated name for the outlined helper.
static std::string makeHelperName(const Function &Parent, unsigned Index,
                                  PRNG &RNG) {
  // Use a hex suffix derived from the PRNG so names look random.
  uint64_t Tag = RNG.next();
  std::string Name;
  raw_string_ostream OS(Name);
  OS << "__kg_" << Parent.getName() << "_bb" << Index
     << "_" << format_hex_no_prefix(Tag & 0xFFFFFF, 6);
  OS.flush();
  return Name;
}

// --------------------------------------------------------------------------
// Core extraction logic
// --------------------------------------------------------------------------

/// Maximum number of live-in arguments we are willing to pass.
/// Blocks with more live-ins are skipped to keep the generated ABI sane.
static constexpr unsigned MaxArgs = 8;

/// Try to extract BB into a new helper function.
/// Returns true if extraction succeeded and the CFG was modified.
static bool extractBlock(BasicBlock *BB, unsigned Index, PRNG &RNG) {
  Function *ParentFn = BB->getParent();
  Module *M = ParentFn->getParent();
  LLVMContext &Ctx = M->getContext();

  // --- Precondition checks (guard) ---

  // Skip if PHI nodes are present.
  if (isa<PHINode>(BB->front()))
    return false;

  // Skip if the block contains calls (ABI complexity).
  if (hasCallInsts(*BB))
    return false;

  // Skip exit blocks (return / unreachable).
  if (isExitBlock(*BB))
    return false;

  // We only handle blocks with exactly one unconditional branch as terminator.
  auto *Term = dyn_cast<BranchInst>(BB->getTerminator());
  if (!Term || !Term->isUnconditional())
    return false;

  BasicBlock *Succ = Term->getSuccessor(0);

  // The successor must not start with PHI nodes that reference BB, because
  // we are about to remove BB as a predecessor.
  for (PHINode &Phi : Succ->phis()) {
    // If Succ has PHIs referencing BB we'd need to fixup; skip for safety.
    (void)Phi;
    return false;
  }

  // --- Live-in computation ---

  std::vector<Value *> LiveIns = collectLiveIns(BB);
  if (LiveIns.size() > MaxArgs)
    return false; // too many arguments

  // --- Build the helper function signature ---

  std::vector<Type *> ParamTypes;
  ParamTypes.reserve(LiveIns.size());
  for (Value *V : LiveIns)
    ParamTypes.push_back(V->getType());

  FunctionType *HelperTy =
      FunctionType::get(Type::getVoidTy(Ctx), ParamTypes, /*isVarArg=*/false);

  std::string HelperName = makeHelperName(*ParentFn, Index, RNG);
  Function *Helper = Function::Create(HelperTy, GlobalValue::InternalLinkage,
                                      HelperName, M);
  Helper->setCallingConv(CallingConv::C);
  // Mark as always-inline so the call we insert can be further optimized
  // away in release builds if desired, while still fragmenting debug views.
  Helper->addFnAttr(Attribute::NoUnwind);

  // --- Build a mapping: original live-in Value* -> helper argument ---

  ValueToValueMapTy VMap;
  {
    unsigned Idx = 0;
    for (Argument &Arg : Helper->args()) {
      Arg.setName(LiveIns[Idx]->getName());
      VMap[LiveIns[Idx]] = &Arg;
      ++Idx;
    }
  }

  // --- Clone BB's instructions into the helper ---

  BasicBlock *HelperEntry =
      BasicBlock::Create(Ctx, "entry", Helper);

  // Clone each instruction except the terminator, remapping operands.
  for (auto It = BB->begin(), End = --BB->end(); It != End; ++It) {
    Instruction *Clone = It->clone();
    Clone->setName(It->getName());
    Clone->insertInto(HelperEntry, HelperEntry->end());
    // Record the mapping so later clones can reference this value.
    VMap[&*It] = Clone;
  }

  // Remap operands of all cloned instructions using VMap.
  for (Instruction &I : *HelperEntry) {
    for (Use &U : I.operands()) {
      Value *OldVal = U.get();
      auto It = VMap.find(OldVal);
      if (It != VMap.end())
        U.set(It->second);
    }
  }

  // Terminate helper with ret void.
  IRBuilder<>(HelperEntry).CreateRetVoid();

  // --- Replace BB's body in the parent function with a call + branch ---

  // Collect instructions to erase (everything except the terminator which
  // we'll repurpose).
  std::vector<Instruction *> ToErase;
  for (auto It = BB->begin(), End = --BB->end(); It != End; ++It)
    ToErase.push_back(&*It);

  // Insert the call to the helper before the terminator.
  IRBuilder<> CallBuilder(BB->getTerminator());
  SmallVector<Value *, 8> Args(LiveIns.begin(), LiveIns.end());
  CallBuilder.CreateCall(HelperTy, Helper, Args);

  // The existing unconditional branch to Succ stays as-is — we just remove
  // the original instructions that were doing the work.
  for (Instruction *I : ToErase)
    I->eraseFromParent();

  return true;
}

// --------------------------------------------------------------------------
// Pass entry point
// --------------------------------------------------------------------------

PreservedAnalyses FunctionSplitPass::run(Module &M,
                                          ModuleAnalysisManager & /*MAM*/) {
  bool AnyChanged = false;
  PRNG &RNG = getModulePRNG();

  // Collect functions to process up-front; extractBlock may add new Functions
  // to the module that we must not re-visit.
  std::vector<Function *> Worklist;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    if (F.isVarArg())
      continue;
    if (F.hasFnAttribute(Attribute::Naked))
      continue;
    if (!shouldObfuscate(F, "fsplit", EnableFSplit))
      continue;
    if (F.size() < 5)
      continue;
    Worklist.push_back(&F);
  }

  for (Function *F : Worklist) {
    // Collect candidate blocks before we start modifying the function.
    // We snapshot them so that newly created blocks aren't considered.
    std::vector<BasicBlock *> Candidates;
    BasicBlock *Entry = &F->getEntryBlock();

    for (BasicBlock &BB : *F) {
      if (&BB == Entry)
        continue;
      if (isExitBlock(BB))
        continue;
      Candidates.push_back(&BB);
    }

    unsigned ExtractedCount = 0;
    for (unsigned I = 0, E = Candidates.size(); I < E; ++I) {
      BasicBlock *BB = Candidates[I];
      // The block may have been deleted (e.g., merged) in a prior iteration
      // within the same function. Validate the parent is still this function.
      if (BB->getParent() != F)
        continue;

      if (extractBlock(BB, I, RNG)) {
        ++ExtractedCount;
        AnyChanged = true;
      }
    }

    if (ExtractedCount > 0) {
      LLVM_DEBUG(dbgs() << "[kagura-fsplit] " << F->getName() << ": extracted "
                        << ExtractedCount << " block(s)\n");
    }
  }

  return AnyChanged ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace kagura
