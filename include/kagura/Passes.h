#pragma once

#include "llvm/IR/PassManager.h"

namespace kagura {

// ---- Control Flow ----

/// Flattens function CFG into a switch-based dispatcher.
struct ControlFlowFlatteningPass
    : public llvm::PassInfoMixin<ControlFlowFlatteningPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                               llvm::FunctionAnalysisManager &FAM);
  static bool isRequired() { return false; }
};

/// Injects bogus control flow with opaque predicates.
struct BogusControlFlowPass
    : public llvm::PassInfoMixin<BogusControlFlowPass> {
  uint32_t Probability = 30; // % of basic blocks to obfuscate
  uint32_t Iterations  = 1;

  BogusControlFlowPass() = default;
  BogusControlFlowPass(uint32_t Prob, uint32_t Iter)
      : Probability(Prob), Iterations(Iter) {}

  llvm::PreservedAnalyses run(llvm::Function &F,
                               llvm::FunctionAnalysisManager &FAM);
  static bool isRequired() { return false; }
};

/// Replaces arithmetic/bitwise ops with equivalent mixed-boolean expressions.
struct SubstitutionPass : public llvm::PassInfoMixin<SubstitutionPass> {
  uint32_t Iterations = 1;

  SubstitutionPass() = default;
  explicit SubstitutionPass(uint32_t Iter) : Iterations(Iter) {}

  llvm::PreservedAnalyses run(llvm::Function &F,
                               llvm::FunctionAnalysisManager &FAM);
  static bool isRequired() { return false; }
};

// ---- Data ----

/// Encrypts string literals at compile time; injects runtime decryption stubs.
struct StringEncryptionPass
    : public llvm::PassInfoMixin<StringEncryptionPass> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

/// Encrypts string literals with AES-128-CTR at compile time.
/// Stronger than XOR-based StringEncryptionPass; requires kagura_runtime.
struct StringEncryptionAESPass
    : public llvm::PassInfoMixin<StringEncryptionAESPass> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

// ---- Mobile Anti-Analysis ----

/// Injects anti-debug / anti-Frida checks (ptrace, port 27042, maps scan).
struct AntiDebugPass : public llvm::PassInfoMixin<AntiDebugPass> {
  bool AntiFramework = true; // check for Frida/Substrate
  bool AntiPtrace    = true;

  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

// ---- iOS specific ----

/// Obfuscates Objective-C selector names and class names in IR metadata.
struct ObjCObfuscationPass : public llvm::PassInfoMixin<ObjCObfuscationPass> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

// ---- Android specific ----

/// Converts static JNI functions (Java_*) to dynamic RegisterNatives calls.
struct JNIObfuscationPass : public llvm::PassInfoMixin<JNIObfuscationPass> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

/// Routes calls to external functions through a runtime-resolved thunk table,
/// defeating static import table analysis (IDA external call resolution).
struct CallIndirectionPass : public llvm::PassInfoMixin<CallIndirectionPass> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

/// Inserts software pointer authentication for function pointers stored in
/// module-level globals, simulating ARM64e PAC on platforms without hardware
/// support via XOR-tagging with a runtime-derived key.
struct PointerAuthPass : public llvm::PassInfoMixin<PointerAuthPass> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

/// Replaces direct function calls with indirect calls through per-callsite
/// function pointer globals, defeating static call graph analysis.
struct IndirectBranchPass : public llvm::PassInfoMixin<IndirectBranchPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                               llvm::FunctionAnalysisManager &FAM);
  static bool isRequired() { return false; }
};

/// Obfuscates loop structures: bogus dead counters, opaque invariant branches,
/// and 64-bit induction variable splitting into i_low / i_high halves.
struct LoopTransformPass : public llvm::PassInfoMixin<LoopTransformPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                               llvm::FunctionAnalysisManager &FAM);
  static bool isRequired() { return false; }
};

// ---- Data obfuscation ----

/// Encrypts non-string integer globals at compile time; patches every load
/// site with an inline XOR to decrypt. Scalar and array-of-integer globals.
struct GlobalEncryptionPass
    : public llvm::PassInfoMixin<GlobalEncryptionPass> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

/// Replaces integer constants with equivalent MBA expressions.
/// e.g. 42  =>  ((x | ~x) & 42) + (x & ~x)  (always evaluates to 42)
struct ConstantObfuscationPass
    : public llvm::PassInfoMixin<ConstantObfuscationPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                               llvm::FunctionAnalysisManager &FAM);
  static bool isRequired() { return false; }
};

// ---- VM Obfuscation ----

/// Virtualizes function bodies into a custom stack-based VM bytecode.
/// The original IR is replaced by an XOR-encrypted bytecode blob and a
/// trampoline that decrypts + dispatches via kagura_vm_execute().
struct VMObfuscationPass : public llvm::PassInfoMixin<VMObfuscationPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                               llvm::FunctionAnalysisManager &FAM);
  static bool isRequired() { return false; }
};

// ---- Anti-tamper ----

/// Injects compile-time FNV-1a integrity hashes and runtime verification calls.
/// Also inserts kagura_self_check() at main() for jailbreak/root detection.
struct AntiTamperPass : public llvm::PassInfoMixin<AntiTamperPass> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

// ---- CFG Fragmentation ----

/// Splits large functions (>= 5 BBs) by extracting eligible interior basic
/// blocks into separate outlined helper functions and replacing each extracted
/// block with a call + unconditional branch to the original successor.
/// Eligible blocks: no PHI nodes, no calls, unconditional branch terminator,
/// successor has no PHI nodes, live-in count <= 8.
struct FunctionSplitPass : public llvm::PassInfoMixin<FunctionSplitPass> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return false; }
};

// ---- CFG Layout ----

/// Randomly shuffles the physical order of basic blocks within a function.
/// CFG edges are unchanged; only the layout is permuted to confuse linear
/// disassemblers and increase reverse-engineering cost.
struct BasicBlockReorderingPass
    : public llvm::PassInfoMixin<BasicBlockReorderingPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                               llvm::FunctionAnalysisManager &FAM);
  static bool isRequired() { return false; }
};

/// Inserts syntactically plausible but semantically dead basic blocks
/// (terminated by `unreachable`) into functions to inflate CFG complexity
/// and mislead static analysis tools.
struct DeadCodeInsertionPass
    : public llvm::PassInfoMixin<DeadCodeInsertionPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                               llvm::FunctionAnalysisManager &FAM);
  static bool isRequired() { return false; }
};

// ---- Metrics ----

/// Collects and prints obfuscation metrics per function:
///   - basic block count delta
///   - instruction count delta
///   - cyclomatic complexity before/after
/// Run this pass BEFORE and AFTER obfuscation passes to compare.
struct ObfuscationMetricsPass
    : public llvm::PassInfoMixin<ObfuscationMetricsPass> {
  bool IsBefore; // true = snapshot before, false = report after

  explicit ObfuscationMetricsPass(bool Before = false) : IsBefore(Before) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return true; } // always run even without opts
};

} // namespace kagura
