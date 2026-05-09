//===-- Plugin.cpp - Kagura LLVM pass plugin registration -----------------===//
//
// Registers all Kagura passes with LLVM's New Pass Manager.
// Load via: clang -fpass-plugin=libKaguraObfuscator.dylib
//
//===----------------------------------------------------------------------===//

#include "kagura/Options.h"
#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/Config/llvm-config.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#if __has_include("llvm/Plugins/PassPlugin.h")
#include "llvm/Plugins/PassPlugin.h"
#else
#include "llvm/Passes/PassPlugin.h"
#endif

using namespace llvm;
using namespace kagura;

//===----------------------------------------------------------------------===//
// Plugin entry point
//===----------------------------------------------------------------------===//

llvm::PassPluginLibraryInfo getKaguraPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "KaguraObfuscator", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // Register individual passes by name (usable with `opt -passes=`)
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) -> bool {
                  if (Name == "kagura-fla") {
                    FPM.addPass(ControlFlowFlatteningPass());
                    return true;
                  }
                  if (Name == "kagura-bcf") {
                    FPM.addPass(BogusControlFlowPass(opt::BCFProb, opt::BCFIter));
                    return true;
                  }
                  if (Name == "kagura-sub") {
                    FPM.addPass(SubstitutionPass(opt::SUBIter));
                    return true;
                  }
                  if (Name == "kagura-co") {
                    FPM.addPass(ConstantObfuscationPass());
                    return true;
                  }
                  if (Name == "kagura-vm") {
                    FPM.addPass(VMObfuscationPass());
                    return true;
                  }
                  if (Name == "kagura-ibr") {
                    FPM.addPass(IndirectBranchPass());
                    return true;
                  }
                  if (Name == "kagura-lt") {
                    FPM.addPass(LoopTransformPass());
                    return true;
                  }
                  if (Name == "kagura-bbr") {
                    FPM.addPass(BasicBlockReorderingPass());
                    return true;
                  }
                  if (Name == "kagura-dci") {
                    FPM.addPass(DeadCodeInsertionPass());
                    return true;
                  }
                  if (Name == "kagura-bbs") {
                    FPM.addPass(BasicBlockSplittingPass());
                    return true;
                  }
                  if (Name == "kagura-anti-debug") {
                    // Handled at module level; no-op here
                    return true;
                  }
                  return false;
                });

            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) -> bool {
                  if (Name == "kagura-ci") {
                    MPM.addPass(CallIndirectionPass());
                    return true;
                  }
                  if (Name == "kagura-pac") {
                    MPM.addPass(PointerAuthPass());
                    return true;
                  }
                  if (Name == "kagura-str") {
                    MPM.addPass(StringEncryptionPass());
                    return true;
                  }
                  if (Name == "kagura-anti-debug") {
                    MPM.addPass(AntiDebugPass());
                    return true;
                  }
                  if (Name == "kagura-objc") {
                    MPM.addPass(ObjCObfuscationPass());
                    return true;
                  }
                  if (Name == "kagura-jni") {
                    MPM.addPass(JNIObfuscationPass());
                    return true;
                  }
                  if (Name == "kagura-fsplit") {
                    MPM.addPass(FunctionSplitPass());
                    return true;
                  }
                  if (Name == "kagura-str-aes") {
                    MPM.addPass(StringEncryptionAESPass());
                    return true;
                  }
                  if (Name == "kagura-tamper") {
                    MPM.addPass(AntiTamperPass());
                    return true;
                  }
                  if (Name == "kagura-genc") {
                    MPM.addPass(GlobalEncryptionPass());
                    return true;
                  }
                  if (Name == "kagura-sv") {
                    MPM.addPass(SymbolVisibilityPass());
                    return true;
                  }
                  return false;
                });

            // Inject at the END of the optimizer pipeline so that LLVM's own
            // passes (instcombine, simplifycfg, etc.) don't undo or crash on
            // our obfuscated IR.
            PB.registerOptimizerLastEPCallback(
#if LLVM_VERSION_MAJOR >= 20
                [](ModulePassManager &MPM, OptimizationLevel OL,
                   ThinOrFullLTOPhase) {
#else
                [](ModulePassManager &MPM, OptimizationLevel OL) {
#endif
                  if (OL == OptimizationLevel::O0)
                    return;
                  // Snapshot BEFORE obfuscation
                  if (opt::Metrics)
                    MPM.addPass(ObfuscationMetricsPass(/*Before=*/true));
                  if (opt::CI)
                    MPM.addPass(CallIndirectionPass());
                  if (opt::PAC)
                    MPM.addPass(PointerAuthPass());
                  if (opt::STR)
                    MPM.addPass(StringEncryptionPass());
                  if (opt::STRAES)
                    MPM.addPass(StringEncryptionAESPass());
                  if (opt::Tamper)
                    MPM.addPass(AntiTamperPass());
                  if (opt::ObjC)
                    MPM.addPass(ObjCObfuscationPass());
                  if (opt::JNI)
                    MPM.addPass(JNIObfuscationPass());
                  if (opt::AntiDebug)
                    MPM.addPass(AntiDebugPass());
                  if (opt::FSplit)
                    MPM.addPass(FunctionSplitPass());
                  if (opt::GENC)
                    MPM.addPass(GlobalEncryptionPass());
                  if (opt::SV)
                    MPM.addPass(SymbolVisibilityPass());

                  FunctionPassManager FPM;
                  bool HasFunctionPass = false;
                  if (opt::FLA) {
                    FPM.addPass(ControlFlowFlatteningPass());
                    HasFunctionPass = true;
                  }
                  if (opt::BCF) {
                    FPM.addPass(BogusControlFlowPass(opt::BCFProb, opt::BCFIter));
                    HasFunctionPass = true;
                  }
                  if (opt::SUB) {
                    FPM.addPass(SubstitutionPass(opt::SUBIter));
                    HasFunctionPass = true;
                  }
                  if (opt::CO) {
                    FPM.addPass(ConstantObfuscationPass());
                    HasFunctionPass = true;
                  }
                  if (opt::VM) {
                    FPM.addPass(VMObfuscationPass());
                    HasFunctionPass = true;
                  }
                  if (opt::IBR) {
                    FPM.addPass(IndirectBranchPass());
                    HasFunctionPass = true;
                  }
                  if (opt::LT) {
                    FPM.addPass(LoopTransformPass());
                    HasFunctionPass = true;
                  }
                  if (opt::BBR) {
                    FPM.addPass(BasicBlockReorderingPass());
                    HasFunctionPass = true;
                  }
                  if (opt::DCI) {
                    FPM.addPass(DeadCodeInsertionPass());
                    HasFunctionPass = true;
                  }
                  if (opt::BBS) {
                    FPM.addPass(BasicBlockSplittingPass());
                    HasFunctionPass = true;
                  }
                  if (HasFunctionPass)
                    MPM.addPass(createModuleToFunctionPassAdaptor(
                        std::move(FPM)));
                  // Snapshot AFTER obfuscation → print report
                  if (opt::Metrics)
                    MPM.addPass(ObfuscationMetricsPass(/*Before=*/false));
                });
          }};
}

// Required export for -fpass-plugin loading
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getKaguraPluginInfo();
}
