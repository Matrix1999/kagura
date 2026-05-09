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
//   -load-kagura  Path to libKaguraObfuscator plugin (default: auto-detect)
//
// Implementation note
// -------------------
// kagura-opt loads the kagura plugin at runtime via PassPlugin::Load() rather
// than linking against it statically.  This avoids the MODULE_LIBRARY linkage
// restriction and header-search-path issues that arise when Plugin.cpp is
// compiled a second time in a different target.  The plugin path is resolved
// from the same directory as the kagura-opt executable (build/bin/../lib/).
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
#include "llvm/Support/Path.h"
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

using namespace llvm;

// ---- CLI options ------------------------------------------------------------

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

static cl::opt<std::string> PluginPath(
    "load-kagura",
    cl::desc("Path to libKaguraObfuscator plugin (default: auto-detect)"),
    cl::init(""));

// ---- Plugin path auto-detection ---------------------------------------------

static std::string findPlugin(const char *Argv0) {
  if (!PluginPath.empty())
    return PluginPath;

  // Try <exe_dir>/../lib/libKaguraObfuscator.{dylib,so}
  SmallString<256> ExeDir(sys::path::parent_path(
      sys::fs::getMainExecutable(Argv0, nullptr)));
  sys::path::append(ExeDir, "..", "lib");

  for (const char *Ext : {"libKaguraObfuscator.dylib",
                           "libKaguraObfuscator.so",
                           "KaguraObfuscator.dll"}) {
    SmallString<256> Candidate(ExeDir);
    sys::path::append(Candidate, Ext);
    if (sys::fs::exists(Candidate))
      return std::string(Candidate);
  }

  // Fallback: look next to the executable
  SmallString<256> ExePath(sys::path::parent_path(
      sys::fs::getMainExecutable(Argv0, nullptr)));
  for (const char *Ext : {"libKaguraObfuscator.dylib",
                           "libKaguraObfuscator.so"}) {
    SmallString<256> Candidate(ExePath);
    sys::path::append(Candidate, Ext);
    if (sys::fs::exists(Candidate))
      return std::string(Candidate);
  }
  return "";
}

// ---- Main -------------------------------------------------------------------

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv,
                               "kagura-opt -- apply kagura obfuscation passes\n");

  // Load the kagura plugin (registers all -kagura-* passes and cl::opts)
  std::string Plugin = findPlugin(argv[0]);
  if (Plugin.empty()) {
    WithColor::error() << "Could not find libKaguraObfuscator plugin.\n"
                       << "Use -load-kagura=<path> to specify it explicitly.\n";
    return 1;
  }

  auto PluginOrErr = PassPlugin::Load(Plugin);
  if (!PluginOrErr) {
    WithColor::error() << "Failed to load plugin " << Plugin << ": "
                       << toString(PluginOrErr.takeError()) << '\n';
    return 1;
  }

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

  // Construct the pass pipeline with the kagura plugin registered.
  PassBuilder PB;
  PluginOrErr->registerPassBuilderCallbacks(PB);

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
