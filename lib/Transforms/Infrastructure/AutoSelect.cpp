//===-- AutoSelect.cpp - Risk-based obfuscation pass auto-selection --------===//
//
// 4.8.1: Analyzes each function's IR characteristics and automatically enables
// the most cost-effective set of kagura passes based on estimated attack surface
// and obfuscation complexity budget.
//
// Selection algorithm:
//   For each function F in the module:
//     1. Compute a risk score from static features:
//          - Cyclomatic complexity  (high → more CFG obfuscation)
//          - Instruction count      (large → avoid VM; use lighter passes)
//          - Presence of string globals referenced from F
//          - Presence of alloca'd integers / pointers (MVO / PE candidates)
//          - External call count    (many externs → CallIndirection candidate)
//     2. Map the score to a protection tier:
//          LOW    (score <  10) → STR + BBR
//          MEDIUM (score < 30)  → STR + BCF + BBR + BBS + MVO
//          HIGH   (score >= 30) → STR + FLA + BCF + BBR + BBS + MVO + PE + SUB
//     3. Apply the tier by setting the per-function "kagura_<pass>" annotation
//        so that shouldObfuscate() picks it up without touching global flags.
//
// This pass is a module pass that runs BEFORE all other kagura passes.
// It never overrides explicit per-function annotations (kagura_nofla etc.)
// or globally-disabled passes.
//
// Pass key:   "kagura-autoselect"
// CLI flag:   -kagura-autoselect
//
//===----------------------------------------------------------------------===//

#include "kagura/Options.h"
#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "kagura-autoselect"

using namespace llvm;

// ---- Risk scoring ----------------------------------------------------------

namespace {

struct RiskFeatures {
  unsigned BBs        = 0;
  unsigned Insts      = 0;
  unsigned Edges      = 0;
  unsigned IntAllocas = 0;  // candidate for MVO
  unsigned PtrAllocas = 0;  // candidate for PE
  unsigned ExtCalls   = 0;  // external function calls
  bool     HasStrings = false;

  unsigned cyclomatic() const {
    return (Edges >= BBs) ? Edges - BBs + 2 : 1;
  }

  // Composite risk score (0–100 scale).
  // Weights are intentionally conservative to avoid over-obfuscating.
  unsigned score() const {
    unsigned S = 0;
    S += std::min(cyclomatic() * 2u, 30u); // up to 30 pts
    S += std::min(Insts / 10u,        20u); // up to 20 pts
    S += std::min(IntAllocas * 3u,    15u); // up to 15 pts
    S += std::min(PtrAllocas * 3u,    15u); // up to 15 pts
    S += HasStrings ? 10u : 0u;             // 10 pts for string refs
    S += std::min(ExtCalls * 2u,      10u); // up to 10 pts
    return S;
  }
};

static RiskFeatures analyze(const Function &F) {
  RiskFeatures R;
  for (const auto &BB : F) {
    ++R.BBs;
    R.Edges += BB.getTerminator()->getNumSuccessors();
    for (const auto &I : BB) {
      ++R.Insts;
      if (const auto *AI = dyn_cast<AllocaInst>(&I)) {
        Type *Ty = AI->getAllocatedType();
        if (Ty->isIntegerTy())  ++R.IntAllocas;
        if (Ty->isPointerTy())  ++R.PtrAllocas;
      }
      if (const auto *CI = dyn_cast<CallInst>(&I)) {
        Function *Callee = CI->getCalledFunction();
        if (Callee && Callee->isDeclaration() && !Callee->isIntrinsic())
          ++R.ExtCalls;
        // Check for references to string globals
        for (unsigned Op = 0; Op < CI->getNumOperands(); ++Op) {
          if (auto *GV = dyn_cast<GlobalVariable>(
                  CI->getOperand(Op)->stripPointerCasts()))
            if (GV->isConstant() && GV->hasInitializer())
              if (auto *CDA = dyn_cast<ConstantDataArray>(GV->getInitializer()))
                if (CDA->isString())
                  R.HasStrings = true;
        }
      }
    }
  }
  return R;
}

enum class ProtectionTier { Low, Medium, High };

static ProtectionTier tierFor(unsigned Score) {
  if (Score < 10) return ProtectionTier::Low;
  if (Score < 30) return ProtectionTier::Medium;
  return ProtectionTier::High;
}

/// Annotate F with the kagura attribute for PassAttr if it isn't already
/// explicitly annotated (neither force-enable nor force-disable).
static void annotateIfAbsent(Function &F, StringRef PassAttr) {
  // Check for existing kagura_<pass> or kagura_no<pass> annotations
  if (kagura::hasAnnotation(F, ("kagura_" + PassAttr).str()))   return;
  if (kagura::hasAnnotation(F, ("kagura_no" + PassAttr).str())) return;

  // Add the annotation via function-level metadata
  LLVMContext &Ctx = F.getContext();
  MDNode *Node = MDNode::get(Ctx, MDString::get(Ctx,
      ("kagura.autoselect." + PassAttr).str()));
  F.setMetadata(("kagura_" + PassAttr).str(), Node);
}

} // anonymous namespace

// ---- Pass entry point -------------------------------------------------------

namespace kagura {

PreservedAnalyses AutoSelectPass::run(Module &M, ModuleAnalysisManager &) {
  if (!kagura::opt::AutoSelect)
    return PreservedAnalyses::all();

  for (auto &F : M) {
    if (F.isDeclaration() || F.isVarArg()) continue;

    RiskFeatures Features = analyze(F);
    unsigned Score = Features.score();
    ProtectionTier Tier = tierFor(Score);

    LLVM_DEBUG(llvm::dbgs()
               << "[kagura-autoselect] " << F.getName()
               << " score=" << Score
               << " cyclo=" << Features.cyclomatic()
               << " insts=" << Features.Insts
               << "\n");

    // Always enable string encryption if the function references strings
    if (Features.HasStrings && kagura::opt::STR)
      annotateIfAbsent(F, "str");

    switch (Tier) {
    case ProtectionTier::Low:
      // Lightweight: BBR only (+ STR above)
      if (kagura::opt::BBR) annotateIfAbsent(F, "bbr");
      break;

    case ProtectionTier::Medium:
      // Moderate: BCF + BBR + BBS + MVO
      if (kagura::opt::BCF) annotateIfAbsent(F, "bcf");
      if (kagura::opt::BBR) annotateIfAbsent(F, "bbr");
      if (kagura::opt::BBS) annotateIfAbsent(F, "bbs");
      if (kagura::opt::MVO && Features.IntAllocas > 0)
        annotateIfAbsent(F, "mvo");
      break;

    case ProtectionTier::High:
      // Heavy: FLA + BCF + BBR + BBS + MVO + PE + SUB
      // Skip FLA and VM for very large functions (> 200 instructions)
      // to avoid excessive code size blowup.
      if (kagura::opt::FLA && Features.Insts <= 200)
        annotateIfAbsent(F, "fla");
      if (kagura::opt::BCF) annotateIfAbsent(F, "bcf");
      if (kagura::opt::BBR) annotateIfAbsent(F, "bbr");
      if (kagura::opt::BBS) annotateIfAbsent(F, "bbs");
      if (kagura::opt::SUB) annotateIfAbsent(F, "sub");
      if (kagura::opt::MVO && Features.IntAllocas > 0)
        annotateIfAbsent(F, "mvo");
      if (kagura::opt::PE && Features.PtrAllocas > 0)
        annotateIfAbsent(F, "pe");
      break;
    }
  }

  return PreservedAnalyses::all(); // annotations only, no IR change
}

} // namespace kagura
