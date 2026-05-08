//===-- Plugin.cpp - Kagura LLVM pass plugin registration -----------------===//
//
// Registers all Kagura passes with LLVM's New Pass Manager.
// Load via: clang -fpass-plugin=libKaguraObfuscator.dylib
//
// Pass pipeline options (append to -mllvm or use with opt):
//   -kagura-fla          Enable CFG flattening
//   -kagura-bcf          Enable bogus control flow
//   -kagura-sub          Enable instruction substitution
//   -kagura-str          Enable string encryption
//   -kagura-anti-debug   Enable anti-debug / anti-Frida injection
//   -kagura-objc         Enable ObjC selector/class obfuscation (iOS)
//   -kagura-jni          Enable JNI dynamic registration (Android)
//   -kagura-bbr          Enable basic block reordering
//   -kagura-dci          Enable dead code insertion
//
//   -kagura-fsplit        Enable function splitting / CFG fragmentation
//
//   -kagura-bcf-prob=<N> Bogus CF probability [0-100] (default: 30)
//   -kagura-bcf-iter=<N> Bogus CF iterations (default: 1)
//   -kagura-sub-iter=<N> Substitution iterations (default: 1)
//   -kagura-dci-prob=<N> Dead code insertion probability [0-100] (default: 40)
//   -kagura-seed=<N>     PRNG seed (0 = system entropy)
//
//===----------------------------------------------------------------------===//

#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

// Exposed for use by Utils.cpp via extern declaration
cl::opt<uint64_t> KaguraSeedOpt("kagura-seed",
                                 cl::desc("[Kagura] PRNG seed (0 = entropy)"),
                                 cl::init(0));

namespace {

// ---- Command-line flags ----

cl::opt<bool> EnableFLA("kagura-fla", cl::desc("[Kagura] CFG flattening"),
                        cl::init(false));
cl::opt<bool> EnableBCF("kagura-bcf",
                        cl::desc("[Kagura] Bogus control flow"),
                        cl::init(false));
cl::opt<bool> EnableSUB("kagura-sub",
                        cl::desc("[Kagura] Instruction substitution"),
                        cl::init(false));
cl::opt<bool> EnableSTR("kagura-str",
                        cl::desc("[Kagura] String encryption"),
                        cl::init(false));
cl::opt<bool> EnableAntiDebug("kagura-anti-debug",
                              cl::desc("[Kagura] Anti-debug / Anti-Frida"),
                              cl::init(false));
cl::opt<bool> EnableObjC("kagura-objc",
                         cl::desc("[Kagura] ObjC obfuscation (iOS)"),
                         cl::init(false));
cl::opt<bool> EnableJNI("kagura-jni",
                        cl::desc("[Kagura] JNI dynamic registration (Android)"),
                        cl::init(false));
cl::opt<bool> EnableCO("kagura-co",
                       cl::desc("[Kagura] Constant obfuscation (MBA)"),
                       cl::init(false));
cl::opt<bool> EnableMetrics("kagura-metrics",
                             cl::desc("[Kagura] Print obfuscation metrics"),
                             cl::init(false));
cl::opt<bool> EnableVM("kagura-vm",
                       cl::desc("[Kagura] VM-based function virtualization"),
                       cl::init(false));
cl::opt<bool> EnableSTRAES("kagura-str-aes",
                            cl::desc("[Kagura] AES-128-CTR string encryption"),
                            cl::init(false));

cl::opt<uint32_t> BCFProb("kagura-bcf-prob",
                           cl::desc("[Kagura] Bogus CF probability [0-100]"),
                           cl::init(30));
cl::opt<uint32_t> BCFIter("kagura-bcf-iter",
                           cl::desc("[Kagura] Bogus CF iterations"),
                           cl::init(1));
cl::opt<uint32_t> SUBIter("kagura-sub-iter",
                           cl::desc("[Kagura] Substitution iterations"),
                           cl::init(1));

} // namespace

// Exposed for use by BasicBlockReordering.cpp and DeadCodeInsertion.cpp
cl::opt<bool> EnableBBR("kagura-bbr",
                         cl::desc("[Kagura] Basic block reordering"),
                         cl::init(false));
cl::opt<bool> EnableDCI("kagura-dci",
                         cl::desc("[Kagura] Dead code insertion"),
                         cl::init(false));
cl::opt<uint32_t> DCIProb("kagura-dci-prob",
                           cl::desc("[Kagura] Dead code insertion probability [0-100]"),
                           cl::init(40));

// Exposed for use by FunctionSplit.cpp via extern declaration
cl::opt<bool> EnableFSplit("kagura-fsplit",
                            cl::desc("[Kagura] Function splitting / CFG fragmentation"),
                            cl::init(false));

// Exposed for use by IndirectBranch.cpp and LoopTransform.cpp (defined there)
// We declare them extern so we can reference them in the callback lambda.
extern cl::opt<bool> EnableIBR;
extern cl::opt<bool> EnableLT;
extern cl::opt<bool> EnableTamper; // defined in AntiTamper.cpp
extern cl::opt<bool> EnableCI;     // defined in CallIndirection.cpp
extern cl::opt<bool> EnablePAC;    // defined in PointerAuth.cpp
extern cl::opt<bool> EnableGENC;   // defined in GlobalEncryption.cpp
// EnableBBR, EnableDCI, DCIProb are defined above (file scope)

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
                    FPM.addPass(kagura::ControlFlowFlatteningPass());
                    return true;
                  }
                  if (Name == "kagura-bcf") {
                    FPM.addPass(
                        kagura::BogusControlFlowPass(BCFProb, BCFIter));
                    return true;
                  }
                  if (Name == "kagura-sub") {
                    FPM.addPass(kagura::SubstitutionPass(SUBIter));
                    return true;
                  }
                  if (Name == "kagura-co") {
                    FPM.addPass(kagura::ConstantObfuscationPass());
                    return true;
                  }
                  if (Name == "kagura-vm") {
                    FPM.addPass(kagura::VMObfuscationPass());
                    return true;
                  }
                  if (Name == "kagura-ibr") {
                    FPM.addPass(kagura::IndirectBranchPass());
                    return true;
                  }
                  if (Name == "kagura-lt") {
                    FPM.addPass(kagura::LoopTransformPass());
                    return true;
                  }
                  if (Name == "kagura-bbr") {
                    FPM.addPass(kagura::BasicBlockReorderingPass());
                    return true;
                  }
                  if (Name == "kagura-dci") {
                    FPM.addPass(kagura::DeadCodeInsertionPass());
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
                    MPM.addPass(kagura::CallIndirectionPass());
                    return true;
                  }
                  if (Name == "kagura-pac") {
                    MPM.addPass(kagura::PointerAuthPass());
                    return true;
                  }
                  if (Name == "kagura-str") {
                    MPM.addPass(kagura::StringEncryptionPass());
                    return true;
                  }
                  if (Name == "kagura-anti-debug") {
                    MPM.addPass(kagura::AntiDebugPass());
                    return true;
                  }
                  if (Name == "kagura-objc") {
                    MPM.addPass(kagura::ObjCObfuscationPass());
                    return true;
                  }
                  if (Name == "kagura-jni") {
                    MPM.addPass(kagura::JNIObfuscationPass());
                    return true;
                  }
                  if (Name == "kagura-fsplit") {
                    MPM.addPass(kagura::FunctionSplitPass());
                    return true;
                  }
                  if (Name == "kagura-str-aes") {
                    MPM.addPass(kagura::StringEncryptionAESPass());
                    return true;
                  }
                  if (Name == "kagura-tamper") {
                    MPM.addPass(kagura::AntiTamperPass());
                    return true;
                  }
                  if (Name == "kagura-genc") {
                    MPM.addPass(kagura::GlobalEncryptionPass());
                    return true;
                  }
                  return false;
                });

            // Inject at the END of the optimizer pipeline so that LLVM's own
            // passes (instcombine, simplifycfg, etc.) don't undo or crash on
            // our obfuscated IR.
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel OL,
                   ThinOrFullLTOPhase) {
                  if (OL == OptimizationLevel::O0)
                    return;
                  // Snapshot BEFORE obfuscation
                  if (EnableMetrics)
                    MPM.addPass(kagura::ObfuscationMetricsPass(/*Before=*/true));
                  if (EnableCI)
                    MPM.addPass(kagura::CallIndirectionPass());
                  if (EnablePAC)
                    MPM.addPass(kagura::PointerAuthPass());
                  if (EnableSTR)
                    MPM.addPass(kagura::StringEncryptionPass());
                  if (EnableSTRAES)
                    MPM.addPass(kagura::StringEncryptionAESPass());
                  if (EnableTamper)
                    MPM.addPass(kagura::AntiTamperPass());
                  if (EnableObjC)
                    MPM.addPass(kagura::ObjCObfuscationPass());
                  if (EnableJNI)
                    MPM.addPass(kagura::JNIObfuscationPass());
                  if (EnableAntiDebug)
                    MPM.addPass(kagura::AntiDebugPass());
                  if (EnableFSplit)
                    MPM.addPass(kagura::FunctionSplitPass());
                  if (EnableGENC)
                    MPM.addPass(kagura::GlobalEncryptionPass());

                  FunctionPassManager FPM;
                  bool HasFunctionPass = false;
                  if (EnableFLA) {
                    FPM.addPass(kagura::ControlFlowFlatteningPass());
                    HasFunctionPass = true;
                  }
                  if (EnableBCF) {
                    FPM.addPass(
                        kagura::BogusControlFlowPass(BCFProb, BCFIter));
                    HasFunctionPass = true;
                  }
                  if (EnableSUB) {
                    FPM.addPass(kagura::SubstitutionPass(SUBIter));
                    HasFunctionPass = true;
                  }
                  if (EnableCO) {
                    FPM.addPass(kagura::ConstantObfuscationPass());
                    HasFunctionPass = true;
                  }
                  if (EnableVM) {
                    FPM.addPass(kagura::VMObfuscationPass());
                    HasFunctionPass = true;
                  }
                  if (EnableIBR) {
                    FPM.addPass(kagura::IndirectBranchPass());
                    HasFunctionPass = true;
                  }
                  if (EnableLT) {
                    FPM.addPass(kagura::LoopTransformPass());
                    HasFunctionPass = true;
                  }
                  if (EnableBBR) {
                    FPM.addPass(kagura::BasicBlockReorderingPass());
                    HasFunctionPass = true;
                  }
                  if (EnableDCI) {
                    FPM.addPass(kagura::DeadCodeInsertionPass());
                    HasFunctionPass = true;
                  }
                  if (HasFunctionPass)
                    MPM.addPass(createModuleToFunctionPassAdaptor(
                        std::move(FPM)));
                  // Snapshot AFTER obfuscation → print report
                  if (EnableMetrics)
                    MPM.addPass(kagura::ObfuscationMetricsPass(/*Before=*/false));
                });
          }};
}

// Required export for -fpass-plugin loading
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getKaguraPluginInfo();
}
