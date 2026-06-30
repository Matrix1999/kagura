//===-- Options.cpp - Kagura CLI option definitions ------------------------===//
//
// Single source of truth for all -kagura-* command-line flags.
// Include "kagura/Options.h" to access them from any pass.
//
//===----------------------------------------------------------------------===//

#include "kagura/Options.h"

using namespace llvm;

namespace kagura {
namespace opt {

// ---- Pass enable flags ----

cl::opt<bool> FLA("kagura-fla",
                  cl::desc("[Kagura] CFG flattening"), cl::init(false));
cl::opt<bool> BCF("kagura-bcf",
                  cl::desc("[Kagura] Bogus control flow"), cl::init(false));
cl::opt<bool> SUB("kagura-sub",
                  cl::desc("[Kagura] Instruction substitution"), cl::init(false));
cl::opt<bool> CSEBreak("kagura-cse-break",
                       cl::desc("[Kagura] Common-subexpression breaker"),
                       cl::init(false));
cl::opt<bool> STR("kagura-str",
                  cl::desc("[Kagura] String encryption"), cl::init(false));
cl::opt<bool> STRAES("kagura-str-aes",
                     cl::desc("[Kagura] AES-128-CTR string encryption"),
                     cl::init(false));
cl::opt<bool> STRSplit("kagura-string-split",
                       cl::desc("[Kagura] Split string literals across multiple globals"),
                       cl::init(false));
cl::opt<bool> AntiDebug("kagura-anti-debug",
                        cl::desc("[Kagura] Anti-debug / Anti-Frida"),
                        cl::init(false));
cl::opt<bool> ObjC("kagura-objc",
                   cl::desc("[Kagura] ObjC obfuscation (iOS)"), cl::init(false));
cl::opt<bool> JNI("kagura-jni",
                  cl::desc("[Kagura] JNI dynamic registration (Android)"),
                  cl::init(false));
cl::opt<bool> CO("kagura-co",
                 cl::desc("[Kagura] Constant obfuscation (MBA)"),
                 cl::init(false));
cl::opt<bool> VM("kagura-vm",
                 cl::desc("[Kagura] VM-based function virtualization"),
                 cl::init(false));
cl::opt<bool> Metrics("kagura-metrics",
                      cl::desc("[Kagura] Print obfuscation metrics"),
                      cl::init(false));
cl::opt<bool> BBR("kagura-bbr",
                  cl::desc("[Kagura] Basic block reordering"), cl::init(false));
cl::opt<bool> DCI("kagura-dci",
                  cl::desc("[Kagura] Dead code insertion"), cl::init(false));
cl::opt<bool> FSplit("kagura-fsplit",
                     cl::desc("[Kagura] Function splitting / CFG fragmentation"),
                     cl::init(false));
cl::opt<bool> BBS("kagura-bbs",
                  cl::desc("[Kagura] Basic block splitting"), cl::init(false));
cl::opt<bool> SV("kagura-sv",
                 cl::desc("[Kagura] Symbol visibility obfuscation"),
                 cl::init(false));
cl::opt<bool> IBR("kagura-ibr",
                  cl::desc("[Kagura] Indirect branching"), cl::init(false));
cl::opt<bool> LT("kagura-lt",
                 cl::desc("[Kagura] Loop transformation"), cl::init(false));
cl::opt<bool> Tamper("kagura-tamper",
                     cl::desc("[Kagura] Anti-tamper: inject runtime integrity checks"),
                     cl::init(false));
cl::opt<bool> CI("kagura-ci",
                 cl::desc("[Kagura] Call indirection"), cl::init(false));
cl::opt<bool> PAC("kagura-pac",
                  cl::desc("[Kagura] Pointer authentication (ARM64)"),
                  cl::init(false));
cl::opt<bool> GENC("kagura-genc",
                   cl::desc("[Kagura] Global variable encryption"),
                   cl::init(false));
cl::opt<bool> PE("kagura-pe",
                 cl::desc("[Kagura] Pointer encryption (address obfuscation)"),
                 cl::init(false));
cl::opt<bool> Telemetry("kagura-telemetry",
                        cl::desc("[Kagura] Inject telemetry event probes"),
                        cl::init(false));
cl::opt<bool> BBCheck("kagura-bbcheck",
                      cl::desc("[Kagura] Basic block opcode checksum guards"),
                      cl::init(false));
cl::opt<bool> VTP("kagura-vtp",
                  cl::desc("[Kagura] RTTI / vtable protection (C++ ABI)"),
                  cl::init(false));
cl::opt<bool> ELT("kagura-elt",
                  cl::desc("[Kagura] Encrypted lookup table (switch encoding)"),
                  cl::init(false));

// ---- Pass tuning parameters ----

cl::opt<uint32_t> BCFProb("kagura-bcf-prob",
                          cl::desc("[Kagura] Bogus CF probability [0-100]"),
                          cl::init(30));
cl::opt<uint32_t> BCFIter("kagura-bcf-iter",
                          cl::desc("[Kagura] Bogus CF iterations"),
                          cl::init(1));
cl::opt<uint32_t> SUBIter("kagura-sub-iter",
                          cl::desc("[Kagura] Substitution iterations"),
                          cl::init(1));
cl::opt<uint32_t> DCIProb("kagura-dci-prob",
                          cl::desc("[Kagura] Dead code insertion probability [0-100]"),
                          cl::init(40));
cl::opt<uint64_t> Seed("kagura-seed",
                       cl::desc("[Kagura] PRNG seed (0 = entropy)"),
                       cl::init(0));

// ---- Phase 4.1 infrastructure flags ----

cl::opt<bool> LTOSafe(
    "kagura-lto-safe",
    cl::desc("[Kagura] Run obfuscation passes during LTO/ThinLTO pipeline"),
    cl::init(false));

cl::opt<bool> O0Protect(
    "kagura-o0-protect",
    cl::desc("[Kagura] Enable lightweight protection at -O0 (debug builds)"),
    cl::init(false));

cl::opt<std::string> DWARFMode(
    "kagura-dwarf",
    cl::desc("[Kagura] Debug info handling: keep (default), strip, obfuscate"),
    cl::init("keep"));

// ---- Phase 4.2 data-protection flags ----

cl::opt<bool> WSTR("kagura-wstr",
                   cl::desc("[Kagura] Wide/UTF-16/CFString encryption"),
                   cl::init(false));

// ---- Phase 4.5 game / anti-cheat flags ----

cl::opt<bool> MVO("kagura-mvo",
                  cl::desc("[Kagura] In-memory value XOR obfuscation"),
                  cl::init(false));

cl::opt<bool> Honey("kagura-honey",
                    cl::desc("[Kagura] Honey value / fake symbol injection"),
                    cl::init(false));

// ---- Phase 4.8 automation flags ----

cl::opt<bool> AutoSelect(
    "kagura-autoselect",
    cl::desc("[Kagura] 4.8.1: Auto-select passes per function based on risk score"),
    cl::init(false));

// ---- Phase 4.6 build-system / DX flags ----

cl::opt<std::string> ConfigFile(
    "kagura-config",
    cl::desc("[Kagura] Path to JSON policy configuration file"),
    cl::init(""));

cl::opt<bool> SymMap("kagura-symmap",
                     cl::desc("[Kagura] Emit symbol map JSON after obfuscation"),
                     cl::init(false));

cl::opt<std::string> SymMapOut(
    "kagura-symmap-out",
    cl::desc("[Kagura] Output path for symbol map (default: kagura_symbols.json)"),
    cl::init(""));

// ---- Phase 4.6.3 / 4.6.4 allowlist / denylist / protect flags ----

cl::opt<std::string> ProtectList(
    "kagura-protect",
    cl::desc("[Kagura] Comma-separated symbol patterns to force-protect"),
    cl::init(""));

cl::opt<std::string> DenyList(
    "kagura-deny",
    cl::desc("[Kagura] Comma-separated symbol/file patterns to exclude from obfuscation"),
    cl::init(""));

cl::opt<std::string> AllowList(
    "kagura-allow",
    cl::desc("[Kagura] Comma-separated allowlist; when set only matching symbols are obfuscated"),
    cl::init(""));

// ---- Phase 4.6.10 audit log ----

cl::opt<bool> AuditLog(
    "kagura-audit",
    cl::desc("[Kagura] Emit an audit log of all protected symbols"),
    cl::init(false));

cl::opt<std::string> AuditLogOut(
    "kagura-audit-out",
    cl::desc("[Kagura] Output path for audit log (default: kagura_audit.json)"),
    cl::init(""));

// ---- Phase 4.2.7 build-time key rotation ----

cl::opt<std::string> BuildID(
    "kagura-build-id",
    cl::desc("[Kagura] Build identifier mixed into PRNG seed for per-build key rotation"),
    cl::init(""));

} // namespace opt
} // namespace kagura
