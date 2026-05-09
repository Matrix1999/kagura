//===-- BogusControlFlow.cpp - Bogus control flow with MBA predicates -----===//
//
// Injects dead basic blocks guarded by opaque predicates that are always true
// but difficult to evaluate statically or symbolically.
//
// Unlike DeClang (which uses FCMP_TRUE — trivially solved by any solver),
// we use Mixed Boolean-Arithmetic (MBA) predicates:
//
//   (x | ~x) == -1          (always true for any x)
//   (x & ~x) == 0           (always true for any x)
//   x*(x+1) % 2 == 0        (product of consecutive ints is always even)
//
// The "true" branch executes the original block; the "false" branch contains
// a semantically equivalent but junk-filled copy (never actually taken).
//
//===----------------------------------------------------------------------===//

#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

using namespace llvm;

namespace kagura {

// ---- Opaque predicate builders ----

// Returns a Value* that is always true (i1 1) regardless of runtime values,
// but is expressed as a non-trivial MBA expression to resist symbolic solvers.
// Uses an existing integer value from the function as the "variable" so that
// solvers cannot immediately constant-fold it.
static Value *buildMBAOpaqueTrue(IRBuilder<> &B, Value *X) {
  LLVMContext &Ctx = B.getContext();
  auto *I32 = Type::getInt32Ty(Ctx);

  // Cast X to i32 if needed
  Value *V = X;
  if (V->getType() != I32) {
    if (V->getType()->isIntegerTy())
      V = B.CreateZExtOrTrunc(V, I32);
    else
      V = ConstantInt::get(I32, 0x12345678);
  }

  // Predicate: (V | ~V) == -1  (always true by De Morgan)
  auto *NotV   = B.CreateNot(V, "bcf.notv");
  auto *OrV    = B.CreateOr(V, NotV, "bcf.or");
  auto *MinusOne = ConstantInt::get(I32, ~0U);
  return B.CreateICmpEQ(OrV, MinusOne, "bcf.pred");
}

// Returns a Value* that is always false: (V & ~V) == 1 (never true)
static Value *buildMBAOpaqueFalse(IRBuilder<> &B, Value *X) {
  LLVMContext &Ctx = B.getContext();
  auto *I32 = Type::getInt32Ty(Ctx);
  Value *V = X;
  if (V->getType() != I32) {
    if (V->getType()->isIntegerTy())
      V = B.CreateZExtOrTrunc(V, I32);
    else
      V = ConstantInt::get(I32, 0xDEADBEEF);
  }
  auto *NotV = B.CreateNot(V);
  auto *AndV = B.CreateAnd(V, NotV);
  return B.CreateICmpNE(AndV, ConstantInt::get(I32, 0));
}

// Pick a "random" existing integer value from the block (first int SSA value).
static Value *pickIntValue(BasicBlock *BB) {
  for (auto &I : *BB)
    if (I.getType()->isIntegerTy() && !isa<PHINode>(&I))
      return &I;
  // Fallback: use a constant
  return ConstantInt::get(Type::getInt32Ty(BB->getContext()), 0xCAFEBABE);
}

// Make a rough clone of BB (only copy non-terminator instructions with junk).
static BasicBlock *makeBogusClone(BasicBlock *BB, Function *F) {
  LLVMContext &Ctx = F->getContext();
  auto *Clone = BasicBlock::Create(Ctx, "bcf.bogus", F);
  IRBuilder<> B(Clone);

  // Copy instructions but replace operands with undef to create junk
  for (auto &I : *BB) {
    if (I.isTerminator())
      continue;
    if (isa<PHINode>(&I))
      continue;
    // Simple junk: just add a nop integer op
    if (I.getType()->isIntegerTy(32)) {
      auto *Undef = UndefValue::get(I.getType());
      B.CreateAdd(Undef, ConstantInt::get(I.getType(), 1), "bcf.junk");
    }
  }
  // Bogus block loops back into itself (never actually entered)
  // Note: use unreachable rather than self-loop to avoid interfering with
  // existing loop structures (e.g., CFG flattening's loop header).
  B.CreateUnreachable();
  return Clone;
}

static bool obfuscateFunction(Function &F, uint32_t Probability,
                               uint32_t Iterations, PRNG &RNG) {
  if (F.isDeclaration() || F.size() < 2)
    return false;

  bool Changed = false;

  for (uint32_t Iter = 0; Iter < Iterations; ++Iter) {
    auto Blocks = getBlocks(F);

    for (auto *BB : Blocks) {
      if (RNG.nextRange(0, 100) >= Probability)
        continue;
      if (BB->getTerminator() == nullptr)
        continue;
      // Don't inject into blocks ending with InvokeInst
      if (isa<InvokeInst>(BB->getTerminator()))
        continue;
      // Don't inject into kagura's own dispatcher/loop blocks
      if (BB->getName().starts_with("kagura."))
        continue;
      // Don't inject into blocks with SwitchInst (flattening dispatcher)
      if (isa<SwitchInst>(BB->getTerminator()))
        continue;

      // Split BB at the first non-PHI / non-debug instruction
      BasicBlock::iterator SplitPoint = BB->getFirstNonPHIOrDbgOrAlloca();
      if (SplitPoint == BB->end())
        continue;

      BasicBlock *OrigTail = BB->splitBasicBlock(SplitPoint, "bcf.orig");

      // Create bogus clone (junk block that loops to itself)
      BasicBlock *BogusBlock = makeBogusClone(OrigTail, &F);

      // Replace BB's unconditional branch with:
      //   if (MBA_opaque_true)  jump OrigTail
      //   else                  jump BogusBlock
      Instruction *OldTerm = BB->getTerminator();
      IRBuilder<> B(OldTerm);
      Value *X    = pickIntValue(BB);
      Value *Cond = buildMBAOpaqueTrue(B, X);
      B.CreateCondBr(Cond, OrigTail, BogusBlock);
      OldTerm->eraseFromParent();

      Changed = true;
    }
  }

  return Changed;
}

PreservedAnalyses BogusControlFlowPass::run(Function &F,
                                             FunctionAnalysisManager &) {
  if (!shouldObfuscate(F, "bcf", true))
    return PreservedAnalyses::all();
  auto &RNG    = getModulePRNG();
  bool Changed = obfuscateFunction(F, Probability, Iterations, RNG);
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace kagura
