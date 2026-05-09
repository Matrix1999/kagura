//===-- tests/fuzz/fuzz_common.h - Shared fuzz harness utilities ----------===//
//
// Helpers used by all kagura libFuzzer fuzz targets.
//
// Each fuzzer:
//   1. Interprets the input bytes as LLVM bitcode (parseIR).
//   2. Runs a specific kagura pass on the parsed module.
//   3. Verifies the module is still valid (verifyModule).
//   4. Returns 0 (continue) on success; libFuzzer handles crash on abort.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Passes/PassBuilder.h"
#if __has_include("llvm/Plugins/PassPlugin.h")
#include "llvm/Plugins/PassPlugin.h"
#else
#include "llvm/Passes/PassPlugin.h"
#endif
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <cstring>
#include <memory>

// Defined in lib/Transforms/Plugin.cpp and linked into fuzz targets.
llvm::PassPluginLibraryInfo getKaguraPluginInfo();

namespace kagura_fuzz {

/// Try to parse `data` as LLVM bitcode or textual IR.
/// Returns nullptr if parsing fails (malformed input — not a crash).
inline std::unique_ptr<llvm::Module>
parseInput(const uint8_t *data, size_t size, llvm::LLVMContext &Ctx) {
    using namespace llvm;
    if (size < 4) return nullptr;

    auto Buf = MemoryBuffer::getMemBuffer(
        StringRef(reinterpret_cast<const char *>(data), size),
        "fuzz_input", /*RequiresNullTerminator=*/false);

    SMDiagnostic Err;
    // parseIR handles both .bc (bitcode) and .ll (textual IR)
    auto M = parseIR(*Buf, Err, Ctx);
    return M; // nullptr if parse failed
}

/// Run verifyModule and return true if the IR is valid.
inline bool verifyIR(llvm::Module &M) {
    std::string Errors;
    llvm::raw_string_ostream OS(Errors);
    return !llvm::verifyModule(M, &OS);
}

} // namespace kagura_fuzz
