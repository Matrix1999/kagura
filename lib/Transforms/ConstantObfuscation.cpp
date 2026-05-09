//===-- ConstantObfuscation.cpp - Integer constant MBA obfuscation --------===//
//
// Replaces integer constant operands with Mixed Boolean-Arithmetic (MBA)
// expressions that always evaluate to the same value but are harder to
// reason about statically.
//
// Supported MBA identities (for 32/64-bit integers, value V):
//   1. V  =>  (V ^ R) ^ R                     (double XOR with random R)
//   2. V  =>  (V + R) - R                     (add/sub with random R)
//   3. V  =>  ((V | ~V) & V)                  (identity via tautology)
//   4. V  =>  (V * R) * modular_inverse(R)    (multiplicative, R must be odd)
//
// Each constant is replaced with a randomly chosen identity.
// Only replaces constants in user functions, not in kagura's own helpers.
//
//===----------------------------------------------------------------------===//

#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace kagura {

// Compute modular multiplicative inverse of odd integer x mod 2^bits
// using extended Euclidean algorithm (Newton's method for 2^n).
static uint64_t modInverse(uint64_t x, unsigned bits) {
  assert(x & 1 && "x must be odd for modular inverse to exist");
  uint64_t inv = 1;
  for (unsigned i = 0; i < bits - 1; ++i)
    inv = inv * (2 - x * inv);
  if (bits < 64)
    inv &= (1ULL << bits) - 1;
  return inv;
}

// Replace a ConstantInt CI (used as operand OpIdx in instruction I) with an
// MBA expression that evaluates to the same value.
static bool obfuscateConstant(Instruction *I, unsigned OpIdx,
                               ConstantInt *CI, PRNG &RNG) {
  // Only handle i8..i64
  unsigned Bits = CI->getBitWidth();
  if (Bits < 8 || Bits > 64)
    return false;

  // Skip zero and all-ones (they appear in many structural patterns)
  uint64_t Val = CI->getZExtValue();
  if (Val == 0 || Val == ((Bits == 64) ? ~0ULL : (1ULL << Bits) - 1))
    return false;

  IRBuilder<> B(I);
  Type *Ty = CI->getType();

  // Pick a random identity transformation
  uint32_t Choice = RNG.next32() % 4;
  Value *Result = nullptr;

  switch (Choice) {
  case 0: {
    // (V ^ R) ^ R
    uint64_t R = RNG.next() & ((Bits == 64) ? ~0ULL : (1ULL << Bits) - 1);
    auto *RC  = ConstantInt::get(Ty, R);
    auto *Xor1 = B.CreateXor(CI, RC, "co.xr1");
    Result     = B.CreateXor(Xor1, RC, "co.xr2");
    break;
  }
  case 1: {
    // (V + R) - R
    uint64_t R = RNG.next() & ((Bits == 64) ? ~0ULL : (1ULL << Bits) - 1);
    auto *RC   = ConstantInt::get(Ty, R);
    auto *Add  = B.CreateAdd(CI, RC, "co.ar1");
    Result     = B.CreateSub(Add, RC, "co.ar2");
    break;
  }
  case 2: {
    // ((V | ~V) & V)  — always == V since (V | ~V) == all-ones
    auto *NotV = B.CreateNot(CI, "co.notv");
    auto *Or   = B.CreateOr(CI, NotV, "co.or");
    Result     = B.CreateAnd(Or, CI, "co.and");
    break;
  }
  case 3: {
    // (V * R) * R_inv   where R is a random odd number
    uint64_t Mask  = (Bits == 64) ? ~0ULL : (1ULL << Bits) - 1;
    uint64_t R     = (RNG.next() | 1) & Mask; // force odd
    uint64_t RInv  = modInverse(R, Bits) & Mask;
    auto *RC       = ConstantInt::get(Ty, R);
    auto *RInvC    = ConstantInt::get(Ty, RInv);
    auto *Mul1     = B.CreateMul(CI, RC, "co.mr1");
    Result         = B.CreateMul(Mul1, RInvC, "co.mr2");
    break;
  }
  }

  if (!Result)
    return false;

  I->setOperand(OpIdx, Result);
  return true;
}

static bool obfuscateFunction(Function &F, PRNG &RNG) {
  bool Changed = false;

  for (auto &BB : F) {
    if (BB.getName().starts_with("kagura."))
      continue;
    for (auto &I : BB) {
      if (isa<PHINode>(&I) || isa<AllocaInst>(&I))
        continue;
      for (unsigned OpIdx = 0; OpIdx < I.getNumOperands(); ++OpIdx) {
        auto *CI = dyn_cast<ConstantInt>(I.getOperand(OpIdx));
        if (!CI)
          continue;
        // Don't touch GEP indices, call targets, etc.
        if (isa<GetElementPtrInst>(&I))
          continue;
        if (isa<CallInst>(&I) || isa<InvokeInst>(&I))
          continue;
        // 30% chance per constant to avoid code bloat
        if (RNG.nextRange(0, 100) >= 30)
          continue;
        Changed |= obfuscateConstant(&I, OpIdx, CI, RNG);
      }
    }
  }

  return Changed;
}

PreservedAnalyses ConstantObfuscationPass::run(Function &F,
                                                FunctionAnalysisManager &) {
  if (!shouldObfuscate(F, "co", true))
    return PreservedAnalyses::all();
  auto &RNG    = getModulePRNG();
  bool Changed = obfuscateFunction(F, RNG);
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace kagura
