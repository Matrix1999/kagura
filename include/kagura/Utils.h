#pragma once

#include "llvm/IR/Function.h"
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

} // namespace kagura
