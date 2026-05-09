//===-- ConfigLoader.cpp - JSON policy file loader -------------------------===//
//
// 4.6.1: Reads a JSON/YAML policy file and applies per-module and
// per-function protection settings, overriding command-line defaults.
//
// File format (JSON):
//
//   {
//     "profile": "BALANCED",          // FAST | BALANCED | STRONG (4.6.2)
//     "passes": {                      // override individual pass enables
//       "fla": true,
//       "bcf": false,
//       "str": true,
//       "str_aes": false,
//       "wstr": true,
//       "genc": true,
//       "mvo": true,
//       "honey": false,
//       "dwarf": "strip"
//     },
//     "tuning": {
//       "bcf_prob": 30,
//       "bcf_iter": 1,
//       "sub_iter": 2,
//       "dci_prob": 40
//     },
//     "allowlist": ["main", "JNI_OnLoad"],
//     "denylist":  ["test_*", "debug_*"]
//   }
//
// 4.6.2: Profile presets
//   FAST     — STR only; no CFG passes
//   BALANCED — STR + BCF(20%) + BBR + BBS + GENC
//   STRONG   — all passes at maximum settings
//
// The loader runs as a module pass BEFORE all other passes so that it can
// adjust the global opt:: flags before they are read by subsequent passes.
//
// Pass key:   "kagura-config"
// CLI flag:   -kagura-config=<path>
//
//===----------------------------------------------------------------------===//

#include "kagura/Options.h"
#include "kagura/Passes.h"

#include "llvm/IR/Module.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace kagura {

// ---- Profile presets (4.6.2) -----------------------------------------------

static void applyProfile(StringRef Profile) {
  if (Profile.equals_insensitive("fast")) {
    opt::STR    = true;
    opt::STRAES = false;
    opt::WSTR   = false;
    opt::FLA    = false;
    opt::BCF    = false;
    opt::BBR    = false;
    opt::BBS    = false;
    opt::DCI    = false;
    opt::SUB    = false;
    opt::CO     = false;
    opt::GENC   = false;
    opt::MVO    = false;
  } else if (Profile.equals_insensitive("balanced")) {
    opt::STR    = true;
    opt::STRAES = false;
    opt::WSTR   = true;
    opt::BCF    = true;
    opt::BCFProb = 20;
    opt::BCFIter = 1;
    opt::BBR    = true;
    opt::BBS    = true;
    opt::DCI    = true;
    opt::GENC   = true;
    opt::MVO    = true;
    opt::FLA    = false;
    opt::SUB    = false;
    opt::CO     = false;
  } else if (Profile.equals_insensitive("strong")) {
    opt::STR    = true;
    opt::STRAES = true;
    opt::WSTR   = true;
    opt::FLA    = true;
    opt::BCF    = true;
    opt::BCFProb = 50;
    opt::BCFIter = 2;
    opt::BBR    = true;
    opt::BBS    = true;
    opt::DCI    = true;
    opt::SUB    = true;
    opt::SUBIter = 2;
    opt::CO     = true;
    opt::GENC   = true;
    opt::MVO    = true;
    opt::Honey  = true;
    opt::LT     = true;
    opt::IBR    = true;
    opt::SV     = true;
  }
  // "custom" or unknown: no-op (use individual CLI flags)
}

// ---- JSON policy loader ----------------------------------------------------

static void applyPassesObject(const json::Object &Passes) {
  auto getBool = [&](StringRef Key, cl::opt<bool> &Flag) {
    if (auto V = Passes.getBoolean(Key))
      Flag = *V;
  };
  getBool("fla",    opt::FLA);
  getBool("bcf",    opt::BCF);
  getBool("sub",    opt::SUB);
  getBool("str",    opt::STR);
  getBool("str_aes",opt::STRAES);
  getBool("wstr",   opt::WSTR);
  getBool("co",     opt::CO);
  getBool("vm",     opt::VM);
  getBool("ibr",    opt::IBR);
  getBool("lt",     opt::LT);
  getBool("bbr",    opt::BBR);
  getBool("dci",    opt::DCI);
  getBool("bbs",    opt::BBS);
  getBool("fsplit", opt::FSplit);
  getBool("sv",     opt::SV);
  getBool("genc",   opt::GENC);
  getBool("mvo",    opt::MVO);
  getBool("honey",  opt::Honey);
  getBool("tamper", opt::Tamper);
  getBool("pac",    opt::PAC);
  getBool("ci",     opt::CI);
  getBool("anti_debug", opt::AntiDebug);
  getBool("objc",   opt::ObjC);
  getBool("jni",    opt::JNI);

  if (auto DwarfVal = Passes.getString("dwarf"))
    opt::DWARFMode = DwarfVal->str();
}

static void applyTuningObject(const json::Object &Tuning) {
  auto getU32 = [&](StringRef Key, cl::opt<uint32_t> &Flag) {
    if (auto V = Tuning.getInteger(Key))
      Flag = static_cast<uint32_t>(*V);
  };
  getU32("bcf_prob", opt::BCFProb);
  getU32("bcf_iter", opt::BCFIter);
  getU32("sub_iter", opt::SUBIter);
  getU32("dci_prob", opt::DCIProb);

  if (auto Seed = Tuning.getInteger("seed"))
    opt::Seed = static_cast<uint64_t>(*Seed);
}

// ---- Pass entry point -------------------------------------------------------

PreservedAnalyses ConfigLoaderPass::run(Module &M, ModuleAnalysisManager &) {
  StringRef ConfigPath = kagura::opt::ConfigFile;
  if (ConfigPath.empty())
    return PreservedAnalyses::all();

  // Load file
  auto BufOrErr = MemoryBuffer::getFile(ConfigPath);
  if (!BufOrErr) {
    errs() << "[kagura] ConfigLoader: cannot open " << ConfigPath
           << ": " << BufOrErr.getError().message() << "\n";
    return PreservedAnalyses::all();
  }

  // Parse JSON
  auto JsonOrErr = json::parse((*BufOrErr)->getBuffer());
  if (!JsonOrErr) {
    errs() << "[kagura] ConfigLoader: JSON parse error in " << ConfigPath
           << ": " << toString(JsonOrErr.takeError()) << "\n";
    return PreservedAnalyses::all();
  }

  auto *Root = JsonOrErr->getAsObject();
  if (!Root) {
    errs() << "[kagura] ConfigLoader: root must be a JSON object\n";
    return PreservedAnalyses::all();
  }

  // 4.6.2: Apply profile preset first (individual overrides below can override)
  if (auto Profile = Root->getString("profile"))
    applyProfile(*Profile);

  // 4.6.1: Apply pass enables / disables
  if (auto *PassesObj = Root->getObject("passes"))
    applyPassesObject(*PassesObj);

  // 4.6.1: Apply tuning parameters
  if (auto *TuningObj = Root->getObject("tuning"))
    applyTuningObject(*TuningObj);

  return PreservedAnalyses::all(); // flags only, no IR modification
}

} // namespace kagura
