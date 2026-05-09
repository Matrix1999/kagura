//===-- tests/fuzz/fuzz_bcf.cpp - Fuzz BogusControlFlowPass ---------------===//
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

    kagura::opt::BCF = true;
    kagura::opt::BCFProb = 30;
    kagura::opt::BCFIter = 1;

    FunctionPassManager FPM;
    FPM.addPass(kagura::BogusControlFlowPass(30, 1));

    FunctionAnalysisManager FAM;
    FAM.registerPass([&] { return PassInstrumentationAnalysis(); });

    for (auto &F : *M) {
        if (F.isDeclaration()) continue;
        FPM.run(F, FAM);
    }

    (void)kagura_fuzz::verifyIR(*M);
    return 0;
}
