//===-- tests/fuzz/fuzz_fla.cpp - Fuzz ControlFlowFlatteningPass ----------===//
//
// 4.7.5: libFuzzer target for ControlFlowFlatteningPass.
// Verifies that the pass does not crash or corrupt IR on arbitrary input.
//
//===----------------------------------------------------------------------===//

#include "fuzz_common.h"
#include "kagura/Options.h"
#include "kagura/Passes.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

using namespace llvm;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    LLVMContext Ctx;
    auto M = kagura_fuzz::parseInput(data, size, Ctx);
    if (!M) return 0; // malformed input — not a crash

    // Enable the pass
    kagura::opt::FLA = true;

    FunctionPassManager FPM;
    FPM.addPass(kagura::ControlFlowFlatteningPass());

    FunctionAnalysisManager FAM;
    FAM.registerPass([&] { return PassInstrumentationAnalysis(); });

    for (auto &F : *M) {
        if (F.isDeclaration()) continue;
        FPM.run(F, FAM);
    }

    // Verify IR integrity
    (void)kagura_fuzz::verifyIR(*M);
    return 0;
}
