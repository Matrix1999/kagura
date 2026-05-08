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

// ---- Data obfuscation ----

/// Replaces integer constants with equivalent MBA expressions.
/// e.g. 42  =>  ((x | ~x) & 42) + (x & ~x)  (always evaluates to 42)
struct ConstantObfuscationPass
    : public llvm::PassInfoMixin<ConstantObfuscationPass> {
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
