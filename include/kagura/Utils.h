#pragma once

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include <cstdint>
#include <string>
#include <vector>

namespace kagura {

// ---- Annotation helpers ----

/// Returns true if F has the given annotation attribute.
bool hasAnnotation(llvm::Function &F, llvm::StringRef Attr);

/// Returns true if F should be obfuscated by the pass identified by PassAttr.
/// Respects both module-level flags and per-function annotations:
///   annotate("kagura_<passAttr>")   -> force enable
///   annotate("kagura_no<passAttr>") -> force disable
bool shouldObfuscate(llvm::Function &F, llvm::StringRef PassAttr,
                     bool GlobalFlag);

// ---- Pseudo-random number generator ----

/// Simple splitmix64-based PRNG seeded once per compilation unit.
class PRNG {
public:
  explicit PRNG(uint64_t Seed = 0);
  uint64_t next();
  uint64_t nextRange(uint64_t Lo, uint64_t Hi); // [Lo, Hi)
  uint32_t next32();

private:
  uint64_t State;
};

/// Returns the module-level PRNG (seeded from -kagura-seed or system entropy).
PRNG &getModulePRNG();

// ---- IR helpers ----

/// Demote all PHI nodes in F to alloca/load/store (needed before flattening).
void demotePhis(llvm::Function &F);

/// Collects all basic blocks in F except entry (safe to iterate & mutate).
std::vector<llvm::BasicBlock *> getBlocks(llvm::Function &F);

/// Get or declare an external C function in M (idempotent).
llvm::Function *getOrDeclare(llvm::Module &M, llvm::StringRef Name,
                             llvm::FunctionType *FTy);

// ---- Exception-handling safety ----

/// Returns true if F contains any invoke or landingpad instructions.
/// Passes that restructure the CFG must skip such functions unless they
/// explicitly handle exception-handling edges.
bool hasExceptionHandling(const llvm::Function &F);

/// Returns true if BB is a landing pad block (starts with LandingPadInst or
/// CleanupPadInst) or contains a catchpad/cleanuppad instruction.
bool isEHBlock(const llvm::BasicBlock &BB);

// ---- Target triple helpers ----

enum class TargetArch {
  ARM64,   // AArch64 (including arm64e)
  ARM64e,  // AArch64 with hardware PAC (apple-arm64e)
  ARMv7,   // 32-bit ARM
  X86_64,  // x86-64
  Other,
};

/// Return the architecture of the module's target triple.
TargetArch getTargetArch(const llvm::Module &M);

/// Returns true if the module targets AArch64 (arm64 or arm64e).
bool isAArch64Target(const llvm::Module &M);

/// Returns true if the module targets arm64e (hardware PAC available).
bool isArm64eTarget(const llvm::Module &M);

/// Returns true if the module targets a 32-bit ARM device.
bool isARMv7Target(const llvm::Module &M);

/// Returns true if the module targets x86-64.
bool isX86_64Target(const llvm::Module &M);

// ---- String global collection ----

/// Collect all ConstantDataArray globals that look like strings, are used in
/// at least one function, and satisfy basic heuristics (length >= 4, no format
/// specifiers, etc.).  If StrictLinkage is true, only private/internal globals
/// are collected.
std::vector<llvm::GlobalVariable *>
collectStringGlobals(llvm::Module &M, bool StrictLinkage = false);

// ---- Constant builders ----

/// Build a ConstantArray of i8 from raw bytes.
llvm::Constant *buildByteArrayConstant(llvm::LLVMContext &Ctx,
                                       llvm::ArrayRef<uint8_t> Data);

/// Create a private constant global holding the given byte data.
llvm::GlobalVariable *createPrivateByteGlobal(llvm::Module &M,
                                              llvm::ArrayRef<uint8_t> Data,
                                              llvm::StringRef Name,
                                              bool IsConstant = true);

/// Fill a buffer with random bytes from the module PRNG.
void fillRandomBytes(uint8_t *Out, size_t Len);

} // namespace kagura
