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

} // namespace opt
} // namespace kagura
