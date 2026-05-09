#include "kagura/Options.h"
#include "kagura/Utils.h"

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
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

PRNG &getModulePRNG() {
  static bool Seeded = false;
  if (!Seeded) {
    GlobalPRNG = PRNG(opt::Seed);
    Seeded = true;
  }
  return GlobalPRNG;
}

// ---- Exception-handling safety ----

bool hasExceptionHandling(const Function &F) {
  for (const BasicBlock &BB : F)
    for (const Instruction &I : BB)
      if (isa<InvokeInst>(I) || isa<LandingPadInst>(I) ||
          isa<ResumeInst>(I) || isa<CleanupPadInst>(I) ||
          isa<CatchPadInst>(I) || isa<CatchReturnInst>(I) ||
          isa<CleanupReturnInst>(I))
        return true;
  return false;
}

bool isEHBlock(const BasicBlock &BB) {
  const Instruction *First = &BB.front();
  if (isa<LandingPadInst>(First) || isa<CleanupPadInst>(First) ||
      isa<CatchPadInst>(First))
    return true;
  for (const Instruction &I : BB)
    if (isa<LandingPadInst>(I) || isa<CleanupPadInst>(I) ||
        isa<CatchPadInst>(I) || isa<ResumeInst>(I))
      return true;
  return false;
}

// ---- Target triple helpers ----

TargetArch getTargetArch(const Module &M) {
  // getTargetTriple() return type changed across LLVM versions:
  //   LLVM 17-19: const std::string &   (no .str() member)
  //   LLVM 20+  : const Triple &        (.str() returns std::string)
  // Use a helper lambda to produce a uniform std::string.
#if LLVM_VERSION_MAJOR >= 20
  std::string TripleStr = M.getTargetTriple().str();
#else
  std::string TripleStr = M.getTargetTriple();
#endif
  StringRef Triple(TripleStr);
  // arm64e must be checked before generic aarch64 since it is a substring match.
  if (Triple.contains("arm64e"))
    return TargetArch::ARM64e;
  if (Triple.starts_with("aarch64") || Triple.starts_with("arm64"))
    return TargetArch::ARM64;
  if (Triple.starts_with("armv7") || Triple.starts_with("thumbv7"))
    return TargetArch::ARMv7;
  if (Triple.starts_with("x86_64") || Triple.starts_with("amd64"))
    return TargetArch::X86_64;
  return TargetArch::Other;
}

bool isAArch64Target(const Module &M) {
  TargetArch A = getTargetArch(M);
  return A == TargetArch::ARM64 || A == TargetArch::ARM64e;
}

bool isArm64eTarget(const Module &M) {
  return getTargetArch(M) == TargetArch::ARM64e;
}

bool isARMv7Target(const Module &M) {
  return getTargetArch(M) == TargetArch::ARMv7;
}

bool isX86_64Target(const Module &M) {
  return getTargetArch(M) == TargetArch::X86_64;
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

Function *getOrDeclare(Module &M, StringRef Name, FunctionType *FTy) {
  if (auto *F = M.getFunction(Name))
    return F;
  return Function::Create(FTy, Function::ExternalLinkage, Name, M);
}

// ---- String global collection ----

std::vector<GlobalVariable *> collectStringGlobals(Module &M,
                                                    bool StrictLinkage) {
  std::vector<GlobalVariable *> Result;
  for (auto &GV : M.globals()) {
    if (!GV.isConstant() || !GV.hasInitializer())
      continue;
    if (StrictLinkage &&
        GV.getLinkage() != GlobalValue::PrivateLinkage &&
        GV.getLinkage() != GlobalValue::InternalLinkage)
      continue;
    auto *CDA = dyn_cast<ConstantDataArray>(GV.getInitializer());
    if (!CDA || !CDA->isString())
      continue;
    StringRef S = CDA->getAsString();
    if (S.size() < 4)
      continue;
    if (S.contains('%') || S.trim().empty())
      continue;
    bool UsedInFunction = false;
    for (auto *U : GV.users()) {
      if (isa<Instruction>(U) ||
          (isa<ConstantExpr>(U) && !cast<ConstantExpr>(U)->user_empty())) {
        UsedInFunction = true;
        break;
      }
    }
    if (UsedInFunction)
      Result.push_back(&GV);
  }
  return Result;
}

// ---- Constant builders ----

Constant *buildByteArrayConstant(LLVMContext &Ctx, ArrayRef<uint8_t> Data) {
  auto *Int8Ty = Type::getInt8Ty(Ctx);
  auto *ArrTy  = ArrayType::get(Int8Ty, Data.size());
  std::vector<Constant *> Bytes;
  Bytes.reserve(Data.size());
  for (uint8_t B : Data)
    Bytes.push_back(ConstantInt::get(Int8Ty, B));
  return ConstantArray::get(ArrTy, Bytes);
}

GlobalVariable *createPrivateByteGlobal(Module &M, ArrayRef<uint8_t> Data,
                                         StringRef Name, bool IsConstant) {
  LLVMContext &Ctx = M.getContext();
  auto *Init = buildByteArrayConstant(Ctx, Data);
  return new GlobalVariable(M, Init->getType(), IsConstant,
                            GlobalValue::PrivateLinkage, Init, Name);
}

void fillRandomBytes(uint8_t *Out, size_t Len) {
  PRNG &RNG = getModulePRNG();
  for (size_t I = 0; I < Len; I += 8) {
    uint64_t V = RNG.next();
    size_t N = std::min(static_cast<size_t>(8), Len - I);
    for (size_t J = 0; J < N; ++J)
      Out[I + J] = static_cast<uint8_t>((V >> (J * 8)) & 0xFF);
  }
}

} // namespace kagura
