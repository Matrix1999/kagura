//===-- ObjCObfuscation.cpp - Objective-C metadata obfuscation ------------===//
//
// Obfuscates Objective-C selector names and class names that appear as
// GlobalVariable string literals in the following Mach-O sections:
//
//   __TEXT,__objc_methnames   (selector strings, e.g. "initWithFoo:bar:")
//   __TEXT,__objc_classnames  (class name strings, e.g. "MyViewController")
//   __TEXT,__objc_methname    (variant section name)
//
// Strategy:
//   1. Collect all GlobalVariables in these sections.
//   2. For each selector/class name, generate a scrambled replacement.
//   3. Update the GlobalVariable's name and section annotation to match.
//   4. Record the mapping in a side table (kagura_objc_map) for runtime lookup.
//
// Note: This pass operates at the IR level.  Mach-O section attributes
// (the __attribute__((section(...))) annotations) are preserved.
// The runtime library (runtime/objc_runtime.m) performs the actual selector
// registration fixup at load time using the mapping table.
//
//===----------------------------------------------------------------------===//

#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"

#include <map>
#include <string>

using namespace llvm;

namespace kagura {

// Returns true if GV is in an ObjC method/class name section
static bool isObjCNameSection(const GlobalVariable &GV) {
  if (!GV.hasSection())
    return false;
  StringRef Section = GV.getSection();
  return Section.contains("__objc_methname") ||
         Section.contains("__objc_classname") ||
         Section.contains("__objc_methnames");
}

// Generate a short alphanumeric obfuscated name
static std::string scrambleName(StringRef Original, PRNG &RNG) {
  static const char Chars[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  constexpr size_t CharsLen = sizeof(Chars) - 1;

  // Preserve selector colons so ObjC ABI stays intact:
  // "initWithFoo:bar:" -> "xK7m:q2R:"
  std::string Result;
  for (char C : Original) {
    if (C == ':') {
      Result += ':';
    } else {
      Result += Chars[RNG.nextRange(0, CharsLen)];
    }
  }
  return Result;
}

PreservedAnalyses ObjCObfuscationPass::run(Module &M,
                                            ModuleAnalysisManager &) {
  LLVMContext &Ctx = M.getContext();
  auto &RNG        = getModulePRNG();

  // name mapping: original -> obfuscated
  std::map<std::string, std::string> NameMap;

  SmallVector<GlobalVariable *, 32> Targets;
  for (auto &GV : M.globals()) {
    if (!GV.hasInitializer() || !isObjCNameSection(GV))
      continue;
    auto *CDA = dyn_cast<ConstantDataArray>(GV.getInitializer());
    if (!CDA || !CDA->isString())
      continue;
    Targets.push_back(&GV);
  }

  if (Targets.empty())
    return PreservedAnalyses::all();

  auto *Int8Ty = Type::getInt8Ty(Ctx);

  for (auto *GV : Targets) {
    auto *CDA = cast<ConstantDataArray>(GV->getInitializer());
    std::string Original = CDA->getAsString().str();
    // Strip null terminator for processing
    if (!Original.empty() && Original.back() == '\0')
      Original.pop_back();

    std::string Obfuscated;
    auto It = NameMap.find(Original);
    if (It != NameMap.end()) {
      Obfuscated = It->second;
    } else {
      Obfuscated = scrambleName(Original, RNG);
      NameMap[Original] = Obfuscated;
    }

    // Build new ConstantDataArray with null terminator
    std::string ObfuscatedZ = Obfuscated + '\0';
    auto *NewConst = ConstantDataArray::getString(Ctx, ObfuscatedZ,
                                                   /*AddNull=*/false);
    auto *NewTy    = ArrayType::get(Int8Ty, ObfuscatedZ.size());

    // Replace the global's initializer with the obfuscated string
    // (same type size assumed; if different, create a new GV)
    if (NewConst->getType() == GV->getInitializer()->getType()) {
      GV->setInitializer(NewConst);
    } else {
      auto *NewGV = new GlobalVariable(
          M, NewTy, GV->isConstant(), GV->getLinkage(), NewConst,
          GV->getName(), GV, GV->getThreadLocalMode(),
          GV->getAddressSpace(), GV->isExternallyInitialized());
      NewGV->setSection(GV->getSection());
      NewGV->setAlignment(GV->getAlign());
      GV->replaceAllUsesWith(NewGV);
      GV->eraseFromParent();
    }
  }

  // Store the mapping table as a module-level global for the runtime
  // Format: null-terminated pairs: original\0obfuscated\0 ... \0
  if (!NameMap.empty()) {
    std::string MapData;
    for (auto &[Orig, Obf] : NameMap) {
      MapData += Orig;
      MapData += '\0';
      MapData += Obf;
      MapData += '\0';
    }
    MapData += '\0'; // double null = end of table

    auto *MapConst = ConstantDataArray::getString(Ctx, MapData, false);
    new GlobalVariable(M, MapConst->getType(), true,
                       GlobalValue::PrivateLinkage, MapConst,
                       "kagura_objc_name_map");
  }

  return PreservedAnalyses::none();
}

} // namespace kagura
