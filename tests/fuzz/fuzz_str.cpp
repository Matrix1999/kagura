//===-- tests/fuzz/fuzz_str.cpp - Fuzz StringEncryptionPass ---------------===//
#include "fuzz_common.h"
#include "kagura/Options.h"
#include "kagura/Passes.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    LLVMContext Ctx;
    auto M = kagura_fuzz::parseInput(data, size, Ctx);
    if (!M) return 0;

    kagura::opt::STR = true;

    ModulePassManager MPM;
    MPM.addPass(kagura::StringEncryptionPass());

    ModuleAnalysisManager MAM;
    MAM.registerPass([&] { return PassInstrumentationAnalysis(); });

    MPM.run(*M, MAM);

    (void)kagura_fuzz::verifyIR(*M);
    return 0;
}
