//===-- BasicBlockReordering.cpp - Shuffle basic block layout -------------===//
//
// Randomly reorders the basic blocks of each function while preserving
// correct CFG semantics.  After reordering, a linear disassembly pass
// (IDA, Ghidra) must follow many unconditional branches to reconstruct
// the original flow, increasing reverse-engineering effort.
//
// Algorithm:
//   1. Collect all BBs except the entry block.
//   2. Fisher-Yates shuffle them.
//   3. Use llvm::BasicBlock::moveAfter() to physically reorder the BBs
//      inside the function's linked list.
//   4. The entry block stays first (required by LLVM ABI).
//
// This pass is purely structural — it does not modify any instructions or
// change the CFG edges, so it is safe to run after other passes.
//
//===----------------------------------------------------------------------===//

#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"

#include <vector>

using namespace llvm;

extern cl::opt<bool> EnableBBR; // defined in Plugin.cpp (file scope)

namespace kagura {

PreservedAnalyses BasicBlockReorderingPass::run(Function &F,
                                                FunctionAnalysisManager &) {
  if (!shouldObfuscate(F, "bbr", EnableBBR))
    return PreservedAnalyses::all();
  if (F.isDeclaration() || F.size() < 3)
    return PreservedAnalyses::all();

  PRNG &RNG = getModulePRNG();

  // Collect non-entry blocks
  std::vector<BasicBlock *> Blocks;
  Blocks.reserve(F.size() - 1);
  for (auto &BB : F) {
    if (&BB == &F.getEntryBlock())
      continue;
    Blocks.push_back(&BB);
  }

  if (Blocks.size() < 2)
    return PreservedAnalyses::all();

  // Fisher-Yates shuffle
  for (size_t I = Blocks.size() - 1; I > 0; --I) {
    size_t J = static_cast<size_t>(RNG.nextRange(0, I + 1));
    std::swap(Blocks[I], Blocks[J]);
  }

  // Reorder: move each block after its predecessor in the new order.
  // Start anchor: the entry block.
  BasicBlock *Prev = &F.getEntryBlock();
  for (BasicBlock *BB : Blocks) {
    BB->moveAfter(Prev);
    Prev = BB;
  }

  // CFG edges are unchanged; only layout metadata is affected.
  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}

} // namespace kagura
