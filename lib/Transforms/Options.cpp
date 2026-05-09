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
cl::opt<bool> STR("kagura-str",
                  cl::desc("[Kagura] String encryption"), cl::init(false));
cl::opt<bool> STRAES("kagura-str-aes",
                     cl::desc("[Kagura] AES-128-CTR string encryption"),
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

} // namespace opt
} // namespace kagura
