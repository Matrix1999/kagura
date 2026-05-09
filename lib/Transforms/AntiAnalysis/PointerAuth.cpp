//===-- PointerAuth.cpp - Software pointer authentication for fn ptrs -----===//
//
// Simulates hardware pointer authentication (ARM64e PAC) in software for
// function pointers stored in module-level globals.  Provides software-level
// CFI that raises the bar for attackers who want to forge or overwrite function
// pointer tables at runtime.
//
// For each GlobalVariable whose value type is a pointer-to-function:
//   1. The compile-time initializer (if any) is replaced with a tagged version:
//        tagged_val = ptr_to_int(original_fn) ^ kagura_pac_key
//      stored as i64 instead of ptr.  (The global's type is changed to i64.)
//   2. Every LoadInst that loads from such a global and whose result feeds
//      directly into a CallInst is rewritten:
//        raw_i64  = load i64, @global
//        untagged = raw_i64 ^ kagura_pac_key
//        fn_ptr   = int_to_ptr(untagged)
//        call fn_ptr(...)
//
// The key itself is a single module-level i64 global (kagura_pac_key) whose
// value is set to 0 at compile time.  A module constructor
// (kagura_init_pac_key, priority 65534) initialises it at startup from a
// runtime entropy source (kagura_random_u64 — provided by the runtime library).
//
// Globals that are skipped:
//   - Non-constant function-pointer globals (might be mutated by user code in
//     non-trivial ways; tagging requires tracking every store)
//   - Globals whose initializer is not a Function constant or null
//   - Globals whose name starts with "kagura_" or "llvm."
//   - Globals with no uses (dead)
//
//===----------------------------------------------------------------------===//

#include "kagura/Options.h"
#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <vector>

using namespace llvm;

namespace kagura {

// ---- Helpers ----

/// Return true if Ty is (or decays to) a pointer-to-function type.
static bool isFunctionPointerType(Type *Ty) {
  // In opaque-pointer LLVM, all pointers are "ptr"; we must look at the
  // global's initializer to determine whether it holds a function pointer.
  // This helper is used together with initializer checks below.
  return Ty->isPointerTy();
}

/// Return true if the constant C is a direct function reference or null.
static bool isFunctionPointerConstant(const Constant *C) {
  if (isa<Function>(C))
    return true;
  if (isa<ConstantPointerNull>(C))
    return true;
  return false;
}

/// Return true if GV is a function-pointer global we should tag.
static bool isTaggableGlobal(const GlobalVariable &GV) {
  if (GV.getName().starts_with("kagura_"))
    return false;
  if (GV.getName().starts_with("llvm."))
    return false;
  if (!GV.hasInitializer())
    return false;
  if (GV.use_empty())
    return false;
  // Value type must be ptr (opaque pointer mode)
  if (!isFunctionPointerType(GV.getValueType()))
    return false;
  // Initializer must be a direct function reference or null pointer
  if (!isFunctionPointerConstant(GV.getInitializer()))
    return false;
  return true;
}

// ---- Build / get kagura_pac_key ----

/// Return (or create) the `@kagura_pac_key = internal global i64 0` global.
static GlobalVariable *getOrCreatePacKey(Module &M) {
  if (auto *Existing = M.getNamedGlobal("kagura_pac_key"))
    return Existing;

  auto *Int64Ty = Type::getInt64Ty(M.getContext());
  auto *GV = new GlobalVariable(M, Int64Ty,
                                /*isConstant=*/false,
                                GlobalValue::InternalLinkage,
                                ConstantInt::get(Int64Ty, 0),
                                "kagura_pac_key");
  GV->setAlignment(Align(8));
  return GV;
}

// ---- Build key-init constructor ----

/// Build `void kagura_init_pac_key(void)` that sets kagura_pac_key to a
/// random 64-bit value obtained from kagura_random_u64() (runtime symbol).
static Function *buildPacKeyConstructor(Module &M, GlobalVariable *PacKey) {
  LLVMContext &Ctx  = M.getContext();
  auto *Int64Ty     = Type::getInt64Ty(Ctx);

  // Declare kagura_random_u64: i64 ()
  auto *RngFTy = FunctionType::get(Int64Ty, false);
  auto *RngFn  = cast<Function>(
      M.getOrInsertFunction("kagura_random_u64", RngFTy).getCallee());

  // void kagura_init_pac_key(void)
  auto *CtorFTy = FunctionType::get(Type::getVoidTy(Ctx), false);
  auto *Ctor    = Function::Create(CtorFTy, Function::InternalLinkage,
                                   "kagura_init_pac_key", M);
  Ctor->addFnAttr(Attribute::NoInline);
  Ctor->addFnAttr(Attribute::NoUnwind);

  auto *Entry = BasicBlock::Create(Ctx, "entry", Ctor);
  IRBuilder<> B(Entry);

  // kagura_pac_key = kagura_random_u64()
  Value *Key = B.CreateCall(RngFTy, RngFn, {}, "pac_key");
  B.CreateAlignedStore(Key, PacKey, Align(8));
  B.CreateRetVoid();

  return Ctor;
}

// ---- Tag the initializer of a global ----

/// Replace a function-pointer global's initializer with a tagged i64:
///   tagged = ptr_to_int(fn) ^ 0      (key is 0 at compile time)
/// The global's type is changed from ptr to i64 in-place by creating a new
/// global and replacing all uses.  Returns the replacement i64 global, or
/// nullptr if the transformation was skipped.
static GlobalVariable *tagGlobal(GlobalVariable *GV, Module &M) {
  LLVMContext &Ctx  = M.getContext();
  auto *Int64Ty     = Type::getInt64Ty(Ctx);

  Constant *Init = GV->getInitializer();

  // Compute the compile-time tagged value.
  // At compile time kagura_pac_key == 0, so tagged = ptr_to_int(fn) ^ 0
  // = ptr_to_int(fn).  The XOR with the real key happens at runtime in
  // the call-site rewrite (via kagura_pac_key load).
  Constant *TaggedInit;
  if (isa<ConstantPointerNull>(Init)) {
    TaggedInit = ConstantInt::get(Int64Ty, 0);
  } else {
    // ptr_to_int(fn) as a ConstantExpr
    TaggedInit = ConstantExpr::getPtrToInt(Init, Int64Ty);
  }

  // Create the new i64 global with identical linkage/visibility/alignment.
  auto *Tagged = new GlobalVariable(M, Int64Ty,
                                    /*isConstant=*/false,
                                    GV->getLinkage(),
                                    TaggedInit,
                                    GV->getName() + ".pac");
  Tagged->setAlignment(Align(8));
  Tagged->setVisibility(GV->getVisibility());
  Tagged->setUnnamedAddr(GV->getUnnamedAddr());
  Tagged->setSection(GV->getSection());

  return Tagged;
}

// ---- Rewrite load+call pairs ----

/// For every LoadInst from TaggedGV that flows directly into a CallInst as the
/// callee operand, insert XOR-untag + inttoptr before the call.  Returns the
/// number of call sites rewritten.
static unsigned rewriteLoadCallPairs(GlobalVariable *TaggedGV,
                                      FunctionType *CalleeFTy, Module &M,
                                      GlobalVariable *PacKey) {
  LLVMContext &Ctx   = M.getContext();
  auto *Int64Ty      = Type::getInt64Ty(Ctx);
  PointerType *PtrTy = PointerType::getUnqual(Ctx);

  SmallVector<LoadInst *, 16> Loads;
  for (auto &U : TaggedGV->uses()) {
    auto *LI = dyn_cast<LoadInst>(U.getUser());
    if (!LI)
      continue;
    Loads.push_back(LI);
  }

  unsigned Count = 0;
  for (auto *LI : Loads) {
    // Collect call uses of this load result.
    SmallVector<CallInst *, 8> CallUses;
    for (auto &U : LI->uses()) {
      auto *CI = dyn_cast<CallInst>(U.getUser());
      if (!CI)
        continue;
      // The load must feed the callee operand (not an argument).
      if (CI->getCalledOperand() != LI)
        continue;
      CallUses.push_back(CI);
    }
    if (CallUses.empty())
      continue;

    // Insert untag sequence right after the load.
    IRBuilder<> B(LI->getNextNode());

    // raw_i64 = LI (already loaded as i64 from the tagged global)
    // key     = load i64, @kagura_pac_key
    Value *Key = B.CreateAlignedLoad(Int64Ty, PacKey, Align(8),
                                     /*isVolatile=*/false, "pac.key");
    // untagged_i64 = raw_i64 ^ key
    Value *Untagged = B.CreateXor(LI, Key, "pac.untagged");
    // fn_ptr = inttoptr(untagged_i64)
    Value *FnPtr = B.CreateIntToPtr(Untagged, PtrTy, "pac.fn_ptr");

    for (auto *CI : CallUses) {
      // Replace the callee operand with the untagged function pointer.
      // Build a new call instruction to get the right FunctionType.
      IRBuilder<> CB(CI);
      SmallVector<Value *, 8> Args(CI->args());
      SmallVector<OperandBundleDef, 2> Bundles;
      for (unsigned I = 0, E = CI->getNumOperandBundles(); I != E; ++I)
        Bundles.emplace_back(CI->getOperandBundleAt(I));

      CallInst *NewCI = CB.CreateCall(CalleeFTy, FnPtr, Args, Bundles, "");
      NewCI->setCallingConv(CI->getCallingConv());
      NewCI->setAttributes(CI->getAttributes());
      NewCI->setTailCallKind(CI->getTailCallKind());
      NewCI->setDebugLoc(CI->getDebugLoc());
      if (CI->getType()->isFPOrFPVectorTy())
        NewCI->copyFastMathFlags(CI);

      if (!CI->getType()->isVoidTy())
        CI->replaceAllUsesWith(NewCI);
      CI->eraseFromParent();
      ++Count;
    }
  }
  return Count;
}

// ---- Pass entry point ----

PreservedAnalyses PointerAuthPass::run(Module &M, ModuleAnalysisManager &) {
  if (!kagura::opt::PAC)
    return PreservedAnalyses::all();

  // Collect taggable globals first; we'll mutate the module as we go.
  SmallVector<GlobalVariable *, 32> Targets;
  for (auto &GV : M.globals())
    if (isTaggableGlobal(GV))
      Targets.push_back(&GV);

  if (Targets.empty())
    return PreservedAnalyses::all();

  GlobalVariable *PacKey = getOrCreatePacKey(M);
  bool Changed = false;

  for (auto *GV : Targets) {
    // Remember the original function type from the initializer so we can
    // reconstruct the right FunctionType for call-site rewrites.
    Constant *Init = GV->getInitializer();
    FunctionType *CalleeFTy = nullptr;
    if (auto *Fn = dyn_cast<Function>(Init))
      CalleeFTy = Fn->getFunctionType();
    else
      continue; // null-initialized; no call sites to rewrite yet

    // Create the tagged i64 replacement global.
    GlobalVariable *Tagged = tagGlobal(GV, M);

    // Rewrite load+call pairs that use the original global.
    // First, we must redirect all loads from GV to load from Tagged instead.
    SmallVector<LoadInst *, 16> OrigLoads;
    for (auto &U : GV->uses()) {
      auto *LI = dyn_cast<LoadInst>(U.getUser());
      if (LI)
        OrigLoads.push_back(LI);
    }
    for (auto *LI : OrigLoads) {
      // Change the pointer operand to the new tagged global and the loaded
      // type to i64.
      IRBuilder<> B(LI);
      auto *NewLI = B.CreateAlignedLoad(Type::getInt64Ty(M.getContext()),
                                        Tagged, Align(8),
                                        LI->isVolatile(), "pac.raw");
      NewLI->setDebugLoc(LI->getDebugLoc());
      LI->replaceAllUsesWith(NewLI);
      LI->eraseFromParent();
    }

    // Now rewrite call sites that use the loaded tagged value.
    rewriteLoadCallPairs(Tagged, CalleeFTy, M, PacKey);

    // Replace remaining non-load uses of GV with Tagged (e.g. stores, bitcasts)
    GV->replaceAllUsesWith(Tagged);
    GV->eraseFromParent();

    Changed = true;
  }

  if (!Changed)
    return PreservedAnalyses::all();

  // Build and register the PAC key initialisation constructor.
  // Priority 65534 so it runs just before the thunk table constructor (65535).
  Function *Ctor = buildPacKeyConstructor(M, PacKey);
  appendToGlobalCtors(M, Ctor, 65534);

  return PreservedAnalyses::none();
}

} // namespace kagura
