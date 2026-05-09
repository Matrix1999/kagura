#pragma once
//===-- Options.h - Kagura CLI flags (centralized declarations) -----------===//
//
// All command-line options are *defined* in Options.cpp and declared here so
// that any pass file can access them via `#include "kagura/Options.h"`.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/CommandLine.h"
#include <cstdint>

namespace kagura {
namespace opt {

// ---- Pass enable flags ----
extern llvm::cl::opt<bool> FLA;
extern llvm::cl::opt<bool> BCF;
extern llvm::cl::opt<bool> SUB;
extern llvm::cl::opt<bool> STR;
extern llvm::cl::opt<bool> STRAES;
extern llvm::cl::opt<bool> AntiDebug;
extern llvm::cl::opt<bool> ObjC;
extern llvm::cl::opt<bool> JNI;
extern llvm::cl::opt<bool> CO;
extern llvm::cl::opt<bool> VM;
extern llvm::cl::opt<bool> Metrics;
extern llvm::cl::opt<bool> BBR;
extern llvm::cl::opt<bool> DCI;
extern llvm::cl::opt<bool> FSplit;
extern llvm::cl::opt<bool> BBS;
extern llvm::cl::opt<bool> SV;
extern llvm::cl::opt<bool> IBR;
extern llvm::cl::opt<bool> LT;
extern llvm::cl::opt<bool> Tamper;
extern llvm::cl::opt<bool> CI;
extern llvm::cl::opt<bool> PAC;
extern llvm::cl::opt<bool> GENC;

// ---- Pass tuning parameters ----
extern llvm::cl::opt<uint32_t> BCFProb;
extern llvm::cl::opt<uint32_t> BCFIter;
extern llvm::cl::opt<uint32_t> SUBIter;
extern llvm::cl::opt<uint32_t> DCIProb;
extern llvm::cl::opt<uint64_t> Seed;

// ---- Phase 4.1 infrastructure flags ----

/// 4.1.1 / 4.1.2: Enable protection during LTO/ThinLTO pipeline phases.
/// When false (default), kagura skips module passes that are unsafe to run
/// during link-time optimisation (e.g. passes that assume single-module IR).
extern llvm::cl::opt<bool> LTOSafe;

/// 4.1.2: Enable a lightweight pass subset at -O0 (debug builds).
/// When false (default), all passes are skipped at O0 for build speed.
extern llvm::cl::opt<bool> O0Protect;

/// 4.1.6: DWARF / debug-info handling mode.
///   "keep"  (default) — preserve all debug info unchanged.
///   "strip" — remove all debug metadata from functions touched by kagura.
///   "obfuscate" — remap source locations to synthetic coordinates.
extern llvm::cl::opt<std::string> DWARFMode;

} // namespace opt
} // namespace kagura
