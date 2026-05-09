<p align="center">
  <img src="https://img.shields.io/badge/LLVM-17%E2%80%9322-blue?style=flat-square" alt="LLVM 17-22">
  <img src="https://img.shields.io/badge/C%2B%2B-17-orange?style=flat-square" alt="C++17">
  <img src="https://img.shields.io/badge/platforms-iOS%20%7C%20Android%20%7C%20macOS%20%7C%20Linux-green?style=flat-square" alt="Platforms">
  <img src="https://img.shields.io/badge/license-MIT-lightgrey?style=flat-square" alt="MIT License">
</p>

# Kagura

> LLVM-based code obfuscation toolkit for iOS and Android native binaries.

Built on the **New Pass Manager** (LLVM 17+). Loaded as a pass plugin via `-fpass-plugin` — no LLVM source tree modification required.

---

## Architecture

```
kagura/
├── include/kagura/         # Public headers (Passes.h, Options.h, Utils.h)
├── lib/Transforms/
│   ├── CFG/                # Control-flow obfuscation passes
│   ├── Data/               # String/constant/global encryption
│   ├── AntiAnalysis/       # Anti-debug, integrity, call indirection
│   ├── Platform/           # iOS (ObjC), Android (JNI), VM virtualization
│   ├── Options.cpp         # Centralized CLI flag definitions
│   ├── Plugin.cpp          # Pass registration & pipeline wiring
│   └── Utils.cpp           # Shared IR helpers & PRNG
├── runtime/                # C runtime library (linked into target binary)
├── integration/            # Xcode, Gradle, Unity, Unreal, CMake toolchains
└── tests/                  # CTest-based pass regression tests
```

---

## Passes

### Control Flow (`CFG/`)

| Flag | Pass | Effect |
|:-----|:-----|:-------|
| `-kagura-fla` | ControlFlowFlattening | Converts CFG into a switch-based state machine |
| `-kagura-bcf` | BogusControlFlow | Injects dead blocks guarded by MBA opaque predicates |
| `-kagura-ibr` | IndirectBranch | Replaces direct calls with loads from function pointer globals |
| `-kagura-ci` | CallIndirection | Routes external calls through a runtime-resolved thunk table |
| `-kagura-lt` | LoopTransform | Adds bogus dead counters and opaque invariant branches |
| `-kagura-fsplit` | FunctionSplit | Extracts interior basic blocks into outlined helper functions |
| `-kagura-bbs` | BasicBlockSplitting | Splits large BBs at random points to inflate CFG complexity |
| `-kagura-bbr` | BasicBlockReordering | Shuffles BB layout to confuse linear disassemblers |
| `-kagura-dci` | DeadCodeInsertion | Inserts unreachable junk blocks to mislead static analysis |

### Data Obfuscation (`Data/`)

| Flag | Pass | Effect |
|:-----|:-----|:-------|
| `-kagura-str` | StringEncryption | XOR-encrypts string literals at compile time |
| `-kagura-str-aes` | StringEncryptionAES | AES-128-CTR string encryption (requires runtime) |
| `-kagura-co` | ConstantObfuscation | Replaces integer constants with MBA expressions |
| `-kagura-sub` | Substitution | Replaces arithmetic/bitwise ops with equivalent MBA |
| `-kagura-genc` | GlobalEncryption | Encrypts private integer globals; inline XOR at load sites |

### Anti-Analysis (`AntiAnalysis/`)

| Flag | Pass | Effect |
|:-----|:-----|:-------|
| `-kagura-anti-debug` | AntiDebug | ptrace, Frida port, `/proc/maps`, hook, breakpoint, emulator checks |
| `-kagura-tamper` | AntiTamper | FNV-1a function checksums + jailbreak/root detection at startup |
| `-kagura-pac` | PointerAuth | Software CFI via XOR-tagged function pointer globals |
| `-kagura-sv` | SymbolVisibility | Sets non-public symbols to hidden; strips from dynamic symtab |

### Platform-Specific (`Platform/`)

| Flag | Pass | Target |
|:-----|:-----|:-------|
| `-kagura-objc` | ObjCObfuscation | iOS — obfuscates ObjC selector and class names in IR metadata |
| `-kagura-jni` | JNIObfuscation | Android — converts static `Java_*` to dynamic `RegisterNatives` |
| `-kagura-vm` | VMObfuscation | Virtualizes function bodies into a custom stack-based VM bytecode |

### Utilities

| Flag | Pass | Effect |
|:-----|:-----|:-------|
| `-kagura-metrics` | ObfuscationMetrics | Prints BB/instruction/cyclomatic complexity delta |

---

## Tuning Parameters

| Option | Default | Description |
|:-------|:--------|:------------|
| `-kagura-seed=<N>` | `0` (entropy) | PRNG seed for reproducible output |
| `-kagura-bcf-prob=<N>` | `30` | Bogus CF probability per BB [0-100] |
| `-kagura-bcf-iter=<N>` | `1` | Bogus CF iterations |
| `-kagura-sub-iter=<N>` | `1` | Substitution iterations |
| `-kagura-dci-prob=<N>` | `40` | Dead code insertion probability [0-100] |
| `-kagura-bbs-min=<N>` | `3` | Min instructions before a BB split point |
| `-kagura-bbs-max-splits=<N>` | `2` | Max splits per basic block |
| `-kagura-sv-keep=<sym>` | -- | Comma-separated symbols to keep visible |

---

## Requirements

- **LLVM 17 - 22** (tested on 17, 18, 19, 21, 22)
- CMake 3.20+
- C++17

---

## Quick Start

### Build

```bash
# macOS (Homebrew LLVM)
brew install llvm
bash build.sh

# Custom LLVM
cmake -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DCMAKE_C_COMPILER=/path/to/clang \
  -DCMAKE_CXX_COMPILER=/path/to/clang++ \
  .
cmake --build build -j$(nproc)
```

Output: `build/lib/Transforms/KaguraObfuscator.dylib` (or `.so` on Linux).

### Usage with clang

```bash
clang -fpass-plugin=build/lib/Transforms/KaguraObfuscator.dylib \
      -mllvm -kagura-str \
      -mllvm -kagura-fla \
      -mllvm -kagura-bcf \
      -mllvm -kagura-bcf-prob=50 \
      -O1 your_file.c -o your_file
```

### Usage with opt (IR-level)

```bash
clang -O1 -emit-llvm -c your_file.c -o your_file.bc

opt --load-pass-plugin=build/lib/Transforms/KaguraObfuscator.dylib \
    -passes="kagura-str,function(kagura-fla,kagura-bcf,kagura-sub)" \
    your_file.bc -o your_file.opt.bc

clang your_file.opt.bc -o your_file
```

### Per-Function Control (annotations)

```c
// Force-enable a pass for this function
__attribute__((annotate("kagura_fla")))
void critical_function(void) { ... }

// Force-disable a pass for this function
__attribute__((annotate("kagura_nofla")))
void performance_sensitive(void) { ... }
```

---

## Recommended Pass Order

The plugin auto-applies this order via `registerOptimizerLastEPCallback`:

```
-O1 / -O2 (standard optimizations first)
  1. kagura-sv          → hide symbols
  2. kagura-str[-aes]   → encrypt strings
  3. kagura-genc        → encrypt globals
  4. kagura-tamper      → integrity hash (before CFG changes)
  5. kagura-ci          → external call indirection
  6. kagura-pac         → pointer auth
  7. kagura-fla         → CFG flattening
  8. kagura-bcf         → bogus control flow
  9. kagura-bbs         → BB splitting
 10. kagura-bbr         → BB reordering
 11. kagura-dci         → dead code insertion
 12. kagura-sub         → instruction substitution
 13. kagura-co          → constant obfuscation
 14. kagura-anti-debug  → anti-analysis checks last
```

---

## Runtime Library

Some passes require linking `libkagura_runtime.a`:

| Pass | Required Symbols |
|:-----|:-----------------|
| StringEncryptionAES | `kagura_aes128_ctr_decrypt` |
| VMObfuscation | `kagura_vm_execute` |
| AntiDebug | `kagura_anti_debug_init`, `kagura_check_hooks`, `kagura_check_breakpoints`, `kagura_check_emulator` |
| AntiTamper | `kagura_self_check`, `kagura_tamper_detected` |
| CallIndirection | `dlsym` (system) |
| PointerAuth | `kagura_random_u64` |

```bash
clang your_file.c build/runtime/libkagura_runtime.a -o your_file
```

---

## Integration

| Platform | Setup |
|:---------|:------|
| **Xcode** | Add `integration/xcode/kagura.xcconfig` + run script phase |
| **Android (Gradle)** | `apply from: "kagura/integration/android/kagura.gradle"` |
| **Unity (IL2CPP)** | Copy `Editor/KaguraPostBuildProcessor.cs` to `Assets/Editor/` |
| **Unreal Engine 5** | Copy `KaguraToolchain.cs` to UBT toolchain path |
| **CMake (Cocos2d-x, etc.)** | `-DCMAKE_TOOLCHAIN_FILE=kagura-toolchain.cmake` |

See [`integration/`](integration/) for detailed per-platform documentation.

---

## Tests

```bash
cd build && ctest --output-on-failure
```

---

## License

MIT — see [LICENSE](LICENSE).
