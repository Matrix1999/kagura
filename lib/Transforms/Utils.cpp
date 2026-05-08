#include "kagura/Utils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"

#include <chrono>
#include <random>

using namespace llvm;

namespace kagura {

// ---- Annotation helpers ----

bool hasAnnotation(Function &F, StringRef Attr) {
  Module *M = F.getParent();
  GlobalVariable *GA =
      M->getGlobalVariable("llvm.global.annotations");
  if (!GA)
    return false;
  auto *CA = dyn_cast<ConstantArray>(GA->getInitializer());
  if (!CA)
    return false;
  for (unsigned I = 0; I < CA->getNumOperands(); ++I) {
    auto *CS = dyn_cast<ConstantStruct>(CA->getOperand(I));
    if (!CS || CS->getNumOperands() < 2)
      continue;
    auto *FPtr =
        dyn_cast<Function>(CS->getOperand(0)->stripPointerCasts());
    if (FPtr != &F)
      continue;
    auto *GV =
        dyn_cast<GlobalVariable>(CS->getOperand(1)->stripPointerCasts());
    if (!GV || !GV->hasInitializer())
      continue;
    auto *Str = dyn_cast<ConstantDataArray>(GV->getInitializer());
    if (!Str)
      continue;
    if (Str->getAsCString().contains(Attr))
      return true;
  }
  return false;
}

bool shouldObfuscate(Function &F, StringRef PassAttr, bool GlobalFlag) {
  // Never obfuscate kagura's own injected helper functions
  if (F.getName().starts_with("kagura_"))
    return false;

  std::string EnableAttr  = ("kagura_" + PassAttr).str();
  std::string DisableAttr = ("kagura_no" + PassAttr).str();
  if (hasAnnotation(F, DisableAttr))
    return false;
  if (hasAnnotation(F, EnableAttr))
    return true;
  return GlobalFlag;
}

// ---- PRNG ----

// Note: the -kagura-seed option is declared in Plugin.cpp to avoid
// duplicate registration. Access via getModulePRNG() which reads it.

PRNG::PRNG(uint64_t Seed) {
  if (Seed == 0) {
    // Mix time-based and std::random_device entropy
    std::random_device RD;
    auto T = std::chrono::steady_clock::now().time_since_epoch().count();
    State = static_cast<uint64_t>(RD()) ^ static_cast<uint64_t>(T);
  } else {
    State = Seed;
  }
}

uint64_t PRNG::next() {
  uint64_t Z = (State += 0x9e3779b97f4a7c15ULL);
  Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebULL;
  return Z ^ (Z >> 31);
}

uint32_t PRNG::next32() { return static_cast<uint32_t>(next()); }

uint64_t PRNG::nextRange(uint64_t Lo, uint64_t Hi) {
  assert(Hi > Lo);
  return Lo + (next() % (Hi - Lo));
}

static PRNG GlobalPRNG(0);

// Declared in Plugin.cpp (anonymous namespace, but accessible via extern)
// We use a plain extern at file scope, outside of namespace kagura.
} // end namespace kagura
extern llvm::cl::opt<uint64_t> KaguraSeedOpt;
namespace kagura {

PRNG &getModulePRNG() {
  static bool Seeded = false;
  if (!Seeded) {
    GlobalPRNG = PRNG(KaguraSeedOpt);
    Seeded = true;
  }
  return GlobalPRNG;
}

// ---- IR helpers ----

void demotePhis(Function &F) {
  std::vector<PHINode *> Phis;
  for (auto &BB : F)
    for (auto &I : BB)
      if (auto *Phi = dyn_cast<PHINode>(&I))
        Phis.push_back(Phi);
  for (auto *Phi : Phis)
    DemotePHIToStack(Phi);
}

std::vector<BasicBlock *> getBlocks(Function &F) {
  std::vector<BasicBlock *> Blocks;
  for (auto &BB : F)
    Blocks.push_back(&BB);
  return Blocks;
}

} // namespace kagura
