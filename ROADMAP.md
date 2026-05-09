# Kagura Roadmap

## Phase 1-3 (Completed)

Phases 1-3 delivered the core obfuscation passes, runtime library, CI/CD, and game engine integrations.

### Implemented Features

- 10 CFG obfuscation passes (FLA, BCF, IBR, CI, LT, FSplit, BBS, BBR, DCI, VM)
- 5 data obfuscation passes (STR, STR-AES, CO, SUB, GENC)
- Anti-analysis (AntiDebug, AntiTamper, PAC, SymbolVisibility)
- Platform-specific (ObjC, JNI)
- Runtime library (AES decryption, VM execution, hook/breakpoint/emulator detection)
- Integration (Xcode, Gradle, Unity IL2CPP, Unreal, CMake toolchain)
- CI (GitHub Actions, LLVM 19/21/22 matrix)
- Metrics reporting

---

## Phase 4: Production-Grade Hardening

Phase 4 elevates kagura from a **research prototype** to a **production-grade protection tool**.

---

### Phase 4.1 — LLVM Infrastructure Hardening

> Goal: Stable operation across all LLVM versions and optimization levels.

| ID | Feature | Priority | Effort | Status |
|:---|:--------|:---------|:-------|:-------|
| 4.1.1 | LTO / ThinLTO pipeline support | High | M | ✅ Done |
| 4.1.2 | O0 mode (lightweight protection for debug builds) | High | S | ✅ Done |
| 4.1.3 | Legacy pass manager compatibility layer (LLVM 14-16) | Low | L | — |
| 4.1.4 | Loop pass separation (move LoopTransform under LoopPassManager) | Med | S | ✅ Done |
| 4.1.5 | Bitcode input support (beyond opt workflows) | Med | S | ✅ Done |
| 4.1.6 | DWARF debug info stripping/transformation control | High | M | ✅ Done |
| 4.1.7 | Target triple dispatch (ARM64 / ARMv7 / x86_64) | High | M | ✅ Done |
| 4.1.8 | arm64e / hardware PAC integration | Med | M | ✅ Done |
| 4.1.9 | Sanitizer compatibility (ASan, TSan, UBSan) | Med | M | ✅ Done |
| 4.1.10 | Exception handling safety (C++ EH, ObjC @try) | High | M | ✅ Done |
| 4.1.11 | RTTI / vtable protection | Med | L | ✅ Done |
| 4.1.12 | Reproducible build / deterministic output verification | High | S | ✅ Done |

**Effort**: S = ~1 day, M = 2-5 days, L = 1-2 weeks, XL = 2+ weeks

---

### Phase 4.2 — Advanced Encryption & Data Protection

> Goal: Full data-type encryption coverage with hardened runtime decryption.

| ID | Feature | Priority | Effort | Status |
|:---|:--------|:---------|:-------|:-------|
| 4.2.1 | Wide string / UTF-16 / CFString encryption | High | M | ✅ Done |
| 4.2.2 | ObjC selector string protection (metadata level) | High | M | ✅ Done |
| 4.2.3 | ObjC class/method name obfuscation | Med | L | ✅ Done |
| 4.2.4 | Lazy decryption (decrypt on first access only) | High | M | ✅ Done |
| 4.2.5 | Short-lived decrypted buffer (zero after use) | High | S | ✅ Done |
| 4.2.6 | Device-bound key derivation (UDID / Android ID) | Med | M | ✅ Done |
| 4.2.7 | Build-time key rotation (unique keys per build) | Med | S | ✅ Done |
| 4.2.8 | Per-customer / per-app variant generation | Low | L | ✅ Done |
| 4.2.9 | Native constant encryption (float/double included) | Med | M | ✅ Done |
| 4.2.10 | Encrypted lookup table (table encoding) | Low | L | ✅ Done |
| 4.2.11 | White-box cryptography | Low | XL | — |
| 4.2.12 | Network endpoint / API key / config blob protection | High | M | ✅ Done |
| 4.2.13 | Checksum-guarded decryption (fail on tamper) | Med | M | ✅ Done |

---

### Phase 4.3 — Anti-Tamper & Integrity

> Goal: Multi-layer detection of binary modification and dynamic attacks.

| ID | Feature | Priority | Effort | Status |
|:---|:--------|:---------|:-------|:-------|
| 4.3.1 | Mach-O structure tampering detection (LC_LOAD_DYLIB, code signature blob) | High | M | ✅ Done |
| 4.3.2 | ELF structure tampering detection (section hash, PHT verification) | High | M | ✅ Done |
| 4.3.3 | iOS code signing status verification (embedded.mobileprovision) | High | M | ✅ Done |
| 4.3.4 | Android APK signature verification (v2/v3/v4) | High | M | ✅ Done |
| 4.3.5 | Dynamic library injection detection (DYLD_INSERT) | High | S | ✅ Done |
| 4.3.6 | Loaded module inspection (suspicious dylib/so) | High | M | ✅ Done |
| 4.3.7 | GOT/PLT hook detection | High | M | ✅ Done |
| 4.3.8 | Symbol interposition detection | Med | M | ✅ Done |
| 4.3.9 | ObjC method swizzling detection | High | M | ✅ Done |
| 4.3.10 | JNI table hook detection | Med | M | ✅ Done |
| 4.3.11 | Direct syscall invocation (hook bypass) | Med | M | ✅ Done |
| 4.3.12 | Hardware breakpoint detection | Med | S | ✅ Done |
| 4.3.13 | Memory page permission check (W+X detection) | Med | S | ✅ Done |
| 4.3.14 | App repackaging detection | High | M | ✅ Done |
| 4.3.15 | Anti-dump / anti-memory-scan | Low | L | ✅ Done |
| 4.3.16 | Basic block level checksum (fine-grained integrity) | Low | L | ✅ Done |

---

### Phase 4.4 — Platform-Specific Hardening

> Goal: Address iOS and Android specific attack vectors.

#### iOS

| ID | Feature | Priority | Effort | Status |
|:---|:--------|:---------|:-------|:-------|
| 4.4.1 | Jailbreak filesystem artifact detection | High | M | ✅ Done |
| 4.4.2 | Cydia/Substrate/FridaGadget.dylib detection | High | S | ✅ Done |
| 4.4.3 | fishhook countermeasure (rebinding detection) | Med | M | ✅ Done |
| 4.4.4 | Swift metadata / demangling countermeasure | Med | L | ✅ Done |
| 4.4.5 | TestFlight vs. production differentiation | Low | S | ✅ Done |
| 4.4.6 | iOS simulator exclusion (TARGET_OS_SIMULATOR) | Med | S | ✅ Done |
| 4.4.7 | Entitlements verification | Med | S | ✅ Done |
| 4.4.8 | App Store review safety validation | High | M | ✅ Done |
| 4.4.9 | dyld image list runtime inspection | Med | S | ✅ Done |

#### Android

| ID | Feature | Priority | Effort | Status |
|:---|:--------|:---------|:-------|:-------|
| 4.4.10 | Magisk/Zygisk detection | High | M | ✅ Done |
| 4.4.11 | Xposed/LSPosed detection | High | M | ✅ Done |
| 4.4.12 | /proc inspection hardening (maps/status/mounts/fd) | Med | M | ✅ Done |
| 4.4.13 | Play Integrity API integration | High | M | ✅ Done |
| 4.4.14 | SafetyNet backward compatibility | Low | S | ✅ Done |
| 4.4.15 | ART/JIT environment considerations | Med | M | ✅ Done |
| 4.4.16 | seccomp/prctl checks | Low | M | ✅ Done |
| 4.4.17 | Native library load order control | Med | S | ✅ Done |
| 4.4.18 | Split APK / AAB support | Med | M | ✅ Done |
| 4.4.19 | libil2cpp.so / libUE4.so specialized protection | Med | M |

---

### Phase 4.5 — Game / Anti-Cheat

> Goal: Game-client specific protection features.

| ID | Feature | Priority | Effort | Status |
|:---|:--------|:---------|:-------|:-------|
| 4.5.1 | Memory value obfuscation (XOR/rotate in-memory) | High | M | ✅ Done |
| 4.5.2 | Pointer encryption (address obfuscation) | Med | M | ✅ Done |
| 4.5.3 | Honey value / decoy variable | High | M | ✅ Done |
| 4.5.4 | Fake function / fake symbol (attacker misdirection) | Med | S | ✅ Done |
| 4.5.5 | State integrity check (client-side invariant) | Med | M | ✅ Done |
| 4.5.6 | Telemetry insertion (cheat signal collection) | Med | M | ✅ Done |
| 4.5.7 | Suspicious behavior logging | Med | M | ✅ Done |
| 4.5.8 | Delayed/soft response design (no immediate ban) | Low | S | ✅ Done |
| 4.5.9 | Integrity report signing + replay prevention | Low | M | ✅ Done |
| 4.5.10 | Nonce/challenge-response (server coordination) | Med | M | ✅ Done |
| 4.5.11 | Damage/hit/cooldown/currency protection (template) | High | M | ✅ Done |
| 4.5.12 | Speed/movement value protection | Med | S | ✅ Done |
| 4.5.13 | Random seed protection | Med | S | ✅ Done |

---

### Phase 4.6 — Build System & Developer Experience

> Goal: Practical usability for large-scale projects and production operations.

| ID | Feature | Priority | Effort | Status |
|:---|:--------|:---------|:-------|:-------|
| 4.6.1 | Config DSL (YAML/JSON policy file) | High | M | ✅ Done |
| 4.6.2 | Obfuscation strength profile (FAST / BALANCED / STRONG) | High | M | ✅ Done |
| 4.6.3 | Annotation/macro-based protection target specification (extended) | High | S | ✅ Done |
| 4.6.4 | Allowlist / denylist (symbol/file/module granularity) | High | M | ✅ Done |
| 4.6.5 | Symbol map output (pre- to post-obfuscation mapping) | High | M | ✅ Done |
| 4.6.6 | Crash symbolication support (dSYM / tombstone) | High | L | ✅ Done |
| 4.6.7 | Incremental build support | Med | L | ✅ Done |
| 4.6.8 | Build cache support | Med | M | ✅ Done |
| 4.6.9 | Multi-flavor support (staging / production) | Med | M | ✅ Done |
| 4.6.10 | Audit log (record of what was protected and how) | Med | S | ✅ Done |
| 4.6.11 | Bazel support | Low | M | ✅ Done |
| 4.6.12 | CocoaPods / SwiftPM support | Med | M | ✅ Done |
| 4.6.13 | CLI tool (config generation / report viewer) | Med | M | ✅ Done |
| 4.6.14 | License management (commercial distribution) | Low | M | ✅ Done |

---

### Phase 4.7 — Testing & Quality

> Goal: Regression prevention and reliability proof.

| ID | Feature | Priority | Effort | Status |
|:---|:--------|:---------|:-------|:-------|
| 4.7.1 | FileCheck lit tests (per-pass IR verification) | High | L | ✅ Done |
| 4.7.2 | Integration tests (real app smoke tests) | High | L | ✅ Done |
| 4.7.3 | LLVM version matrix expansion (add 17, 18) | Med | M | ✅ Done |
| 4.7.4 | NDK version matrix | Med | M | ✅ Done |
| 4.7.5 | Fuzzing (crash detection on pass inputs) | Med | L | ✅ Done |
| 4.7.6 | Differential testing (obfuscated vs. plain behavior match) | High | L | ✅ Done |
| 4.7.7 | Performance benchmark (binary size / startup / frame time) | Med | M | ✅ Done |
| 4.7.8 | Battery impact measurement | Low | M | ✅ Done |
| 4.7.9 | Device farm tests (iOS/Android real devices) | Low | XL | — |
| 4.7.10 | False positive evaluation (detection accuracy on clean apps) | Med | M | ✅ Done |
| 4.7.11 | App Store / Google Play review risk assessment | High | M | ✅ Done |

---

### Phase 4.8 — Research & Advanced

> Goal: Maximize attacker cost and resist automated analysis.

| ID | Feature | Priority | Effort | Status |
|:---|:--------|:---------|:-------|:-------|
| 4.8.1 | Obfuscation transform auto-selection (risk-based) | Med | XL | — |
| 4.8.2 | Hot path avoidance (skip protection on perf-critical paths) | High | M | ✅ Done |
| 4.8.3 | Attacker cost modeling | Low | L | ✅ Done |
| 4.8.4 | Symbolic execution resistance evaluation (angr/Triton) | Med | L | ✅ Done |
| 4.8.5 | Decompiler resistance evaluation (Ghidra/IDA/Binary Ninja) | Med | L | ✅ Done |
| 4.8.6 | Frida script resistance evaluation | Med | M | ✅ Done |
| 4.8.7 | Regression corpus (known-attack reproduction tests) | Med | L | ✅ Done |
| 4.8.8 | Red-team evaluation tooling | Low | L | ✅ Done |
| 4.8.9 | ML/heuristic-based protection target inference | Low | XL | — |
| 4.8.10 | Machine code / backend-level obfuscation | Low | XL | — |

---

## Implementation Priority (Phase 4 First Wave)

High-priority items to tackle in the first sprints:

```
Sprint 1 (4.1 Infrastructure):
  4.1.1  LTO / ThinLTO
  4.1.6  DWARF control
  4.1.7  Target triple dispatch
  4.1.10 Exception handling safety
  4.1.12 Reproducible build verification

Sprint 2 (4.2 + 4.3 Encryption & Integrity):
  4.2.1  Wide string support
  4.2.4  Lazy decryption
  4.2.5  Short-lived buffer
  4.2.12 Endpoint / API key protection
  4.3.1  Mach-O tampering detection
  4.3.4  APK signature verification
  4.3.5  Dylib injection detection
  4.3.7  GOT/PLT hook detection

Sprint 3 (4.4 Platform):
  4.4.1  Jailbreak detection
  4.4.2  Cydia/Substrate detection
  4.4.10 Magisk/Zygisk detection
  4.4.13 Play Integrity integration

Sprint 4 (4.5 + 4.6 Game & DX):
  4.5.1  Memory value obfuscation
  4.5.3  Honey value
  4.5.11 Game logic protection template
  4.6.1  Config DSL
  4.6.2  Strength profile
  4.6.5  Symbol map output

Sprint 5 (4.7 Quality):
  4.7.1  FileCheck tests
  4.7.6  Differential testing
  4.7.11 Review risk assessment
```

---

## Effort Estimates

| Phase | Items | Est. Total |
|:------|:------|:-----------|
| 4.1 Infrastructure | 12 | 3-4 weeks |
| 4.2 Encryption | 13 | 4-6 weeks |
| 4.3 Anti-Tamper | 16 | 4-6 weeks |
| 4.4 Platform | 19 | 5-7 weeks |
| 4.5 Game | 13 | 3-5 weeks |
| 4.6 Build/DX | 14 | 4-6 weeks |
| 4.7 Testing | 11 | 4-6 weeks |
| 4.8 Research | 10 | 6-10 weeks |
| **Total** | **108** | **33-50 weeks** |

---

## Non-Goals (Phase 4)

The following are out of scope for Phase 4:

- Server-side components (dashboard, remote config — deferred to Phase 5)
- GUI / Electron app
- Web-based obfuscation
- Source-level obfuscation (C preprocessor tricks, etc.)
- Java/Kotlin bytecode-level obfuscation (ProGuard/R8 territory)
