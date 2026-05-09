//===-- LegacyPlugin.cpp - Kagura legacy pass manager compatibility --------===//
//
// 4.1.3: Provides thin shim wrappers around New PM passes so that kagura can
// be loaded via LLVM 14-16's legacy pass manager (opt -load / PassManagerBuilder
// extension points).
//
// Enable: compile with -DKAGURA_LEGACY_PM=ON
// Load:   opt -load libKaguraObfuscator.so -kagura-fla -kagura-str ...
//
// Each wrapper is a minimal llvm::FunctionPass / llvm::ModulePass that:
//   1. Creates the corresponding New-PM pass object.
//   2. Builds a trivial FAM / MAM with only the default analyses registered.
//   3. Calls pass.run(F/M, am) and checks PreservedAnalyses.
//
// Limitation: analysis results requested inside the New PM pass fall back to
// a freshly-computed local instance on every run — there is no cross-pass
// caching.  This is acceptable for LLVM 14-16 compatibility mode.
//
//===----------------------------------------------------------------------===//

#if defined(KAGURA_LEGACY_PM) && KAGURA_LEGACY_PM

#include "kagura/Options.h"
#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace kagura;

// ---- Helper: build a minimal FunctionAnalysisManager --------------------

static FunctionAnalysisManager buildFAM(Module &M) {
  PassBuilder PB;
  FunctionAnalysisManager FAM;
  PB.registerFunctionAnalyses(FAM);
  return FAM;
}

static ModuleAnalysisManager buildMAM() {
  PassBuilder PB;
  ModuleAnalysisManager MAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  LoopAnalysisManager LAM;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
  return MAM;
}

// ---- Macro: define a legacy FunctionPass shim ---------------------------
//
//  KAGURA_LEGACY_FPASS(ClassName, PassName, Construction)
//
//    ClassName    — legacy class name (e.g. KaguraFLALegacy)
//    PassName     — opt -passes= name used as legacy pass ID
//    Construction — expression that produces the New-PM pass object

#define KAGURA_LEGACY_FPASS(ClassName, PassIDStr, Construction)               \
  namespace {                                                                   \
  struct ClassName : public FunctionPass {                                      \
    static char ID;                                                            \
    ClassName() : FunctionPass(ID) {}                                          \
    bool runOnFunction(Function &F) override {                                 \
      if (F.isDeclaration()) return false;                                     \
      FunctionAnalysisManager FAM = buildFAM(*F.getParent());                  \
      auto Pass = Construction;                                                 \
      auto PA = Pass.run(F, FAM);                                              \
      return !PA.areAllPreserved();                                             \
    }                                                                          \
    StringRef getPassName() const override { return PassIDStr; }               \
  };                                                                           \
  char ClassName::ID = 0;                                                      \
  } /* anonymous */

// ---- Macro: define a legacy ModulePass shim -----------------------------

#define KAGURA_LEGACY_MPASS(ClassName, PassIDStr, Construction)               \
  namespace {                                                                   \
  struct ClassName : public ModulePass {                                        \
    static char ID;                                                            \
    ClassName() : ModulePass(ID) {}                                            \
    bool runOnModule(Module &M) override {                                     \
      ModuleAnalysisManager MAM = buildMAM();                                  \
      auto Pass = Construction;                                                 \
      auto PA = Pass.run(M, MAM);                                              \
      return !PA.areAllPreserved();                                             \
    }                                                                          \
    StringRef getPassName() const override { return PassIDStr; }               \
  };                                                                           \
  char ClassName::ID = 0;                                                      \
  } /* anonymous */

// ---- Function-level pass shims ------------------------------------------

KAGURA_LEGACY_FPASS(KaguraFLALegacy, "kagura-fla",
                    ControlFlowFlatteningPass())

KAGURA_LEGACY_FPASS(KaguraBCFLegacy, "kagura-bcf",
                    BogusControlFlowPass(opt::BCFProb, opt::BCFIter))

KAGURA_LEGACY_FPASS(KaguraSUBLegacy, "kagura-sub",
                    SubstitutionPass(opt::SUBIter))

KAGURA_LEGACY_FPASS(KaguraCOLegacy, "kagura-co",
                    ConstantObfuscationPass())

KAGURA_LEGACY_FPASS(KaguraIBRLegacy, "kagura-ibr",
                    IndirectBranchPass())

KAGURA_LEGACY_FPASS(KaguraLTLegacy, "kagura-lt",
                    LoopTransformPass())

KAGURA_LEGACY_FPASS(KaguraBBRLegacy, "kagura-bbr",
                    BasicBlockReorderingPass())

KAGURA_LEGACY_FPASS(KaguraBBSLegacy, "kagura-bbs",
                    BasicBlockSplittingPass())

KAGURA_LEGACY_FPASS(KaguraDCILegacy, "kagura-dci",
                    DeadCodeInsertionPass())

KAGURA_LEGACY_FPASS(KaguraMVOLegacy, "kagura-mvo",
                    MemoryValueObfuscationPass())

KAGURA_LEGACY_FPASS(KaguraPELegacy, "kagura-pe",
                    PointerEncryptionPass())

KAGURA_LEGACY_FPASS(KaguraVMLegacy, "kagura-vm",
                    VMObfuscationPass())

KAGURA_LEGACY_FPASS(KaguraBBCheckLegacy, "kagura-bbcheck",
                    BasicBlockChecksumPass())

KAGURA_LEGACY_FPASS(KaguraTelemetryLegacy, "kagura-telemetry",
                    TelemetryPass())

KAGURA_LEGACY_FPASS(KaguraELTLegacy, "kagura-elt",
                    EncryptedLookupTablePass())

// ---- Module-level pass shims --------------------------------------------

KAGURA_LEGACY_MPASS(KaguraSTRLegacy, "kagura-str",
                    StringEncryptionPass())

KAGURA_LEGACY_MPASS(KaguraSTRAESLegacy, "kagura-str-aes",
                    StringEncryptionAESPass())

KAGURA_LEGACY_MPASS(KaguraWSTRLegacy, "kagura-wstr",
                    WideStringEncryptionPass())

KAGURA_LEGACY_MPASS(KaguraAntiDebugLegacy, "kagura-anti-debug",
                    AntiDebugPass())

KAGURA_LEGACY_MPASS(KaguraTamperLegacy, "kagura-tamper",
                    AntiTamperPass())

KAGURA_LEGACY_MPASS(KaguraObjCLegacy, "kagura-objc",
                    ObjCObfuscationPass())

KAGURA_LEGACY_MPASS(KaguraJNILegacy, "kagura-jni",
                    JNIObfuscationPass())

KAGURA_LEGACY_MPASS(KaguraCILegacy, "kagura-ci",
                    CallIndirectionPass())

KAGURA_LEGACY_MPASS(KaguraPACLegacy, "kagura-pac",
                    PointerAuthPass())

KAGURA_LEGACY_MPASS(KaguraGENCLegacy, "kagura-genc",
                    GlobalEncryptionPass())

KAGURA_LEGACY_MPASS(KaguraHoneyLegacy, "kagura-honey",
                    HoneyValuePass())

KAGURA_LEGACY_MPASS(KaguraSVLegacy, "kagura-sv",
                    SymbolVisibilityPass())

KAGURA_LEGACY_MPASS(KaguraFSplitLegacy, "kagura-fsplit",
                    FunctionSplitPass())

KAGURA_LEGACY_MPASS(KaguraDWARFLegacy, "kagura-dwarf-control",
                    DWARFControlPass())

KAGURA_LEGACY_MPASS(KaguraVTPLegacy, "kagura-vtp",
                    VTableProtectionPass())

KAGURA_LEGACY_MPASS(KaguraSymMapLegacy, "kagura-symmap",
                    SymbolMapPass())

KAGURA_LEGACY_MPASS(KaguraAuditLegacy, "kagura-audit",
                    AuditLogPass())

KAGURA_LEGACY_MPASS(KaguraConfigLegacy, "kagura-config",
                    ConfigLoaderPass())

// ---- Registration via PassManagerBuilder --------------------------------
//
// Injects passes at EP_OptimizerLast (mirrors New-PM registerOptimizerLastEP).

static void registerKaguraLegacyPasses(const PassManagerBuilder &,
                                        legacy::PassManagerBase &PM) {
  if (opt::CI)         PM.add(new KaguraCILegacy());
  if (opt::PAC)        PM.add(new KaguraPACLegacy());
  if (opt::STR)        PM.add(new KaguraSTRLegacy());
  if (opt::STRAES)     PM.add(new KaguraSTRAESLegacy());
  if (opt::WSTR)       PM.add(new KaguraWSTRLegacy());
  if (opt::Tamper)     PM.add(new KaguraTamperLegacy());
  if (opt::ObjC)       PM.add(new KaguraObjCLegacy());
  if (opt::JNI)        PM.add(new KaguraJNILegacy());
  if (opt::AntiDebug)  PM.add(new KaguraAntiDebugLegacy());
  if (opt::FSplit)     PM.add(new KaguraFSplitLegacy());
  if (opt::GENC)       PM.add(new KaguraGENCLegacy());
  if (opt::Honey)      PM.add(new KaguraHoneyLegacy());
  if (opt::SV)         PM.add(new KaguraSVLegacy());
  if (opt::FLA)        PM.add(new KaguraFLALegacy());
  if (opt::BCF)        PM.add(new KaguraBCFLegacy());
  if (opt::SUB)        PM.add(new KaguraSUBLegacy());
  if (opt::CO)         PM.add(new KaguraCOLegacy());
  if (opt::VM)         PM.add(new KaguraVMLegacy());
  if (opt::IBR)        PM.add(new KaguraIBRLegacy());
  if (opt::LT)         PM.add(new KaguraLTLegacy());
  if (opt::BBR)        PM.add(new KaguraBBRLegacy());
  if (opt::DCI)        PM.add(new KaguraDCILegacy());
  if (opt::BBS)        PM.add(new KaguraBBSLegacy());
  if (opt::MVO)        PM.add(new KaguraMVOLegacy());
  if (opt::PE)         PM.add(new KaguraPELegacy());
  if (opt::Telemetry)  PM.add(new KaguraTelemetryLegacy());
  if (opt::BBCheck)    PM.add(new KaguraBBCheckLegacy());
  if (opt::ELT)        PM.add(new KaguraELTLegacy());
  if (opt::DWARFMode != "keep") PM.add(new KaguraDWARFLegacy());
  if (opt::SymMap)     PM.add(new KaguraSymMapLegacy());
  if (opt::VTP)        PM.add(new KaguraVTPLegacy());
  if (opt::AuditLog)   PM.add(new KaguraAuditLegacy());
}

static RegisterStandardPasses LegacyRegistration(
    PassManagerBuilder::EP_OptimizerLast,
    registerKaguraLegacyPasses);

// Also expose each pass for explicit `-kagura-*` opt command-line flags.
// Registration here lets `opt -load ... -kagura-fla` work on LLVM 14-16.
static RegisterPass<KaguraFLALegacy>     RegFLA("kagura-fla",
    "Kagura: control flow flattening (legacy PM)");
static RegisterPass<KaguraBCFLegacy>     RegBCF("kagura-bcf",
    "Kagura: bogus control flow (legacy PM)");
static RegisterPass<KaguraSUBLegacy>     RegSUB("kagura-sub",
    "Kagura: instruction substitution (legacy PM)");
static RegisterPass<KaguraCOLegacy>      RegCO("kagura-co",
    "Kagura: constant obfuscation (legacy PM)");
static RegisterPass<KaguraIBRLegacy>     RegIBR("kagura-ibr",
    "Kagura: indirect branching (legacy PM)");
static RegisterPass<KaguraLTLegacy>      RegLT("kagura-lt",
    "Kagura: loop transform (legacy PM)");
static RegisterPass<KaguraBBRLegacy>     RegBBR("kagura-bbr",
    "Kagura: basic block reordering (legacy PM)");
static RegisterPass<KaguraBBSLegacy>     RegBBS("kagura-bbs",
    "Kagura: basic block splitting (legacy PM)");
static RegisterPass<KaguraDCILegacy>     RegDCI("kagura-dci",
    "Kagura: dead code insertion (legacy PM)");
static RegisterPass<KaguraMVOLegacy>     RegMVO("kagura-mvo",
    "Kagura: memory value obfuscation (legacy PM)");
static RegisterPass<KaguraPELegacy>      RegPE("kagura-pe",
    "Kagura: pointer encryption (legacy PM)");
static RegisterPass<KaguraVMLegacy>      RegVM("kagura-vm",
    "Kagura: VM obfuscation (legacy PM)");
static RegisterPass<KaguraBBCheckLegacy> RegBBCheck("kagura-bbcheck",
    "Kagura: basic block checksum (legacy PM)");
static RegisterPass<KaguraTelemetryLegacy> RegTelemetry("kagura-telemetry",
    "Kagura: telemetry (legacy PM)");
static RegisterPass<KaguraELTLegacy>     RegELT("kagura-elt",
    "Kagura: encrypted lookup table (legacy PM)");
static RegisterPass<KaguraSTRLegacy>     RegSTR("kagura-str",
    "Kagura: string encryption (legacy PM)");
static RegisterPass<KaguraSTRAESLegacy>  RegSTRAES("kagura-str-aes",
    "Kagura: AES string encryption (legacy PM)");
static RegisterPass<KaguraWSTRLegacy>    RegWSTR("kagura-wstr",
    "Kagura: wide string encryption (legacy PM)");
static RegisterPass<KaguraAntiDebugLegacy> RegAntiDebug("kagura-anti-debug",
    "Kagura: anti-debug (legacy PM)");
static RegisterPass<KaguraTamperLegacy>  RegTamper("kagura-tamper",
    "Kagura: anti-tamper (legacy PM)");
static RegisterPass<KaguraObjCLegacy>    RegObjC("kagura-objc",
    "Kagura: ObjC obfuscation (legacy PM)");
static RegisterPass<KaguraJNILegacy>     RegJNI("kagura-jni",
    "Kagura: JNI obfuscation (legacy PM)");
static RegisterPass<KaguraCILegacy>      RegCI("kagura-ci",
    "Kagura: call indirection (legacy PM)");
static RegisterPass<KaguraPACLegacy>     RegPAC("kagura-pac",
    "Kagura: pointer auth (legacy PM)");
static RegisterPass<KaguraGENCLegacy>    RegGENC("kagura-genc",
    "Kagura: global encryption (legacy PM)");
static RegisterPass<KaguraHoneyLegacy>   RegHoney("kagura-honey",
    "Kagura: honey values (legacy PM)");
static RegisterPass<KaguraSVLegacy>      RegSV("kagura-sv",
    "Kagura: symbol visibility (legacy PM)");
static RegisterPass<KaguraFSplitLegacy>  RegFSplit("kagura-fsplit",
    "Kagura: function split (legacy PM)");
static RegisterPass<KaguraDWARFLegacy>   RegDWARF("kagura-dwarf-control",
    "Kagura: DWARF control (legacy PM)");
static RegisterPass<KaguraVTPLegacy>     RegVTP("kagura-vtp",
    "Kagura: vtable protection (legacy PM)");
static RegisterPass<KaguraSymMapLegacy>  RegSymMap("kagura-symmap",
    "Kagura: symbol map (legacy PM)");
static RegisterPass<KaguraAuditLegacy>   RegAudit("kagura-audit",
    "Kagura: audit log (legacy PM)");
static RegisterPass<KaguraConfigLegacy>  RegConfig("kagura-config",
    "Kagura: config loader (legacy PM)");

#endif // KAGURA_LEGACY_PM
