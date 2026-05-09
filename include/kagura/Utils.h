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
