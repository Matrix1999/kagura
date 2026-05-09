//===-- tools/kagura-opt.cpp - Standalone bitcode optimizer ----------------===//
//
// 4.1.5: Bitcode input support — standalone tool to apply kagura passes to
// .bc / .ll files without a full clang invocation.
//
// Usage:
//   kagura-opt [options] <input.bc|input.ll> -o <output.bc|output.ll>
//
// Examples:
//   # Control flow flattening + substitution on bitcode
//   kagura-opt -kagura-fla -kagura-sub input.bc -o output.bc
//
//   # String encryption on LLVM IR text
//   kagura-opt -kagura-str input.ll -o output.ll -S
//
//   # All available passes with JSON config
//   kagura-opt -kagura-config policy.json input.bc -o output.bc
//
// Flags:
//   All -kagura-* flags defined in Options.cpp are accepted.
//   -S            Emit human-readable .ll instead of binary .bc
//   -o <path>     Output path (default: stdout)
//   -O<N>         Optimization level forwarded to PassBuilder (0-3, s, z)
//
// The tool builds a PassBuilder pipeline using the same optimizer-last EP
// callback registered in Plugin.cpp and runs it over the input module.
//
//===----------------------------------------------------------------------===//

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"
#if __has_include("llvm/Plugins/PassPlugin.h")
#include "llvm/Plugins/PassPlugin.h"
#else
#include "llvm/Passes/PassPlugin.h"
#endif

#include <memory>

// Forward declaration: defined in lib/Transforms/Plugin.cpp and linked in.
llvm::PassPluginLibraryInfo getKaguraPluginInfo();

using namespace llvm;

// ---- CLI options specific to kagura-opt ----------------------------------

static cl::opt<std::string> InputFilename(
    cl::Positional, cl::desc("<input .bc or .ll>"), cl::Required);

static cl::opt<std::string> OutputFilename(
    "o", cl::desc("Output filename"), cl::value_desc("filename"),
    cl::init("-"));

static cl::opt<bool> OutputTextual(
    "S", cl::desc("Emit text IR (.ll) instead of bitcode (.bc)"),
    cl::init(false));

static cl::opt<char> OptLevel(
    "O", cl::desc("Optimization level (0-3, s, z)"),
    cl::Prefix, cl::init('2'));

// ---- Main ----------------------------------------------------------------

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv,
                               "kagura-opt -- apply kagura obfuscation passes\n");

  // Load the input module
  LLVMContext Ctx;
  SMDiagnostic Err;
  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Ctx);
  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }

  // Build optimization level
  OptimizationLevel OL = OptimizationLevel::O2;
  switch (OptLevel) {
  case '0': OL = OptimizationLevel::O0; break;
  case '1': OL = OptimizationLevel::O1; break;
  case '2': OL = OptimizationLevel::O2; break;
  case '3': OL = OptimizationLevel::O3; break;
  case 's': OL = OptimizationLevel::Os; break;
  case 'z': OL = OptimizationLevel::Oz; break;
  default:  OL = OptimizationLevel::O2; break;
  }

  // Construct the pass pipeline.
  // We register the kagura plugin manually (same as -fpass-plugin) so all
  // -kagura-* flags defined via cl::opt in Options.cpp are honoured.
  PassBuilder PB;
  auto PluginInfo = getKaguraPluginInfo();
  PluginInfo.RegisterPassBuilderCallbacks(PB);

  LoopAnalysisManager     LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager    CGAM;
  ModuleAnalysisManager   MAM;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(OL);
  MPM.run(*M, MAM);

  // Write output
  std::error_code EC;
  sys::fs::OpenFlags OutFlags = sys::fs::OF_None;
  if (OutputTextual)
    OutFlags |= sys::fs::OF_Text;

  ToolOutputFile Out(OutputFilename, EC, OutFlags);
  if (EC) {
    WithColor::error() << EC.message() << '\n';
    return 1;
  }

  if (OutputTextual) {
    Out.os() << *M;
  } else {
    WriteBitcodeToFile(*M, Out.os());
  }

  Out.keep();
  return 0;
}
