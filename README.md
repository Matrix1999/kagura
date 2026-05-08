# kagura

LLVM-based obfuscation toolkit for iOS and Android native code.

Built on the **New Pass Manager** (LLVM 17+). Loaded as a pass plugin via `-fpass-plugin` — no LLVM source tree modification required.

## Features

### Control Flow

| Flag | Pass | Description |
|------|------|-------------|
| `-kagura-fla` | ControlFlowFlattening | Converts CFG into a switch-based state machine |
| `-kagura-bcf` | BogusControlFlow | Injects dead blocks guarded by MBA opaque predicates |
| `-kagura-ibr` | IndirectBranch | Replaces direct calls with indirect calls through function pointer globals |
| `-kagura-ci` | CallIndirection | Routes external calls through a runtime-resolved thunk table |
| `-kagura-lt` | LoopTransform | Adds bogus dead counters and opaque invariant branches to loops |
| `-kagura-fsplit` | FunctionSplit | Extracts interior basic blocks into outlined helper functions |
| `-kagura-vm` | VMObfuscation | Virtualizes function bodies into a custom stack-based VM bytecode |
| `-kagura-bbs` | BasicBlockSplitting | Splits large basic blocks at random points to inflate CFG complexity |
| `-kagura-bbr` | BasicBlockReordering | Shuffles basic block layout to confuse linear disassemblers |
| `-kagura-dci` | DeadCodeInsertion | Inserts unreachable junk blocks to mislead static analysis |

### Data

| Flag | Pass | Description |
|------|------|-------------|
| `-kagura-str` | StringEncryption | XOR-encrypts string literals at compile time |
| `-kagura-str-aes` | StringEncryptionAES | AES-128-CTR string encryption (requires `kagura_runtime`) |
| `-kagura-co` | ConstantObfuscation | Replaces integer constants with MBA expressions |
| `-kagura-sub` | Substitution | Replaces arithmetic/bitwise ops with equivalent MBA expressions |
| `-kagura-genc` | GlobalEncryption | Encrypts private integer globals; patches load sites with inline XOR |

### CFI / Pointer Protection

| Flag | Pass | Description |
|------|------|-------------|
| `-kagura-pac` | PointerAuth | Software CFI via XOR-tagged function pointer globals (simulates ARM64e PAC) |

### Anti-Analysis

| Flag | Pass | Description |
|------|------|-------------|
| `-kagura-anti-debug` | AntiDebug | ptrace, Frida port, `/proc/maps`, inline hook, breakpoint, and emulator checks |
| `-kagura-tamper` | AntiTamper | FNV-1a function checksums and jailbreak/root detection at startup |

### Symbol & Visibility

| Flag | Pass | Description |
|------|------|-------------|
| `-kagura-sv` | SymbolVisibility | Sets non-public symbols to hidden visibility; strips them from the dynamic symbol table |

### Platform-Specific

| Flag | Pass | Platform |
|------|------|----------|
| `-kagura-objc` | ObjCObfuscation | Obfuscates ObjC selector and class names in IR metadata | iOS |
| `-kagura-jni` | JNIObfuscation | Converts static `Java_*` functions to dynamic `RegisterNatives` | Android |

### Utilities

| Flag | Pass | Description |
|------|------|-------------|
| `-kagura-metrics` | ObfuscationMetrics | Prints BB/instruction/cyclomatic complexity delta per function |

## Requirements

- LLVM 17 or later (tested on 17, 18, 19)
- CMake 3.20+
- C++17

## Build

```bash
# macOS with Homebrew LLVM
brew install llvm

bash build.sh
```

The plugin is output to `build/lib/Transforms/KaguraObfuscator.dylib`.

For a custom LLVM installation:

```bash
cmake -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DCMAKE_C_COMPILER=/path/to/llvm/bin/clang \
  -DCMAKE_CXX_COMPILER=/path/to/llvm/bin/clang++ \
  .
cmake --build build -- -j$(nproc)
```

## Usage

### clang

```bash
clang -fpass-plugin=build/lib/Transforms/KaguraObfuscator.dylib \
      -mllvm -kagura-str \
      -mllvm -kagura-fla \
      -mllvm -kagura-bcf \
      -mllvm -kagura-bcf-prob=50 \
      -O1 your_file.c -o your_file
```

### opt (IR-level)

```bash
# Compile to IR
clang -O1 -emit-llvm -c your_file.c -o your_file.bc

# Apply passes
opt -load-pass-plugin=build/lib/Transforms/KaguraObfuscator.dylib \
    -passes="kagura-fla,kagura-bcf,kagura-sub" \
    your_file.bc -o your_file.opt.bc

# Compile to native
clang your_file.opt.bc -o your_file
```

### Options

| Option | Default | Description |
|--------|---------|-------------|
| `-kagura-seed=<N>` | `0` (entropy) | PRNG seed; set for reproducible output |
| `-kagura-bcf-prob=<N>` | `30` | Bogus CF probability per basic block [0–100] |
| `-kagura-bcf-iter=<N>` | `1` | Bogus CF iterations |
| `-kagura-sub-iter=<N>` | `1` | Substitution iterations |
| `-kagura-dci-prob=<N>` | `40` | Dead code insertion probability per block [0–100] |
| `-kagura-bbs-min=<N>` | `3` | Minimum instructions before a BB split point |
| `-kagura-bbs-max-splits=<N>` | `2` | Maximum splits per basic block |
| `-kagura-sv-keep=<sym>` | — | Comma-separated symbols to keep visible (repeatable) |

## Integration

### Xcode

Add `integration/xcode/kagura.xcconfig` to your target's build configuration, then add `integration/xcode/kagura-build-phase.sh` as a Run Script phase **before** Compile Sources.

See [`integration/xcode/`](integration/xcode/) for details.

### Android (Gradle + CMake)

```groovy
// app/build.gradle
apply from: "${rootDir}/../kagura/integration/android/kagura.gradle"
```

Optionally override settings in `local.properties`:

```properties
kagura.pluginPath=/path/to/KaguraObfuscator.so
```

See [`integration/android/`](integration/android/) for all options.

## Game Engine Integration

### Unity (IL2CPP)

Copy `integration/unity/Editor/KaguraPostBuildProcessor.cs` into `Assets/Editor/`.
Configure via **Edit > Project Settings > Kagura Obfuscator**.

See [`integration/unity/`](integration/unity/) for full setup and the IL2CPP
runtime protection API (`kagura_il2cpp_*`).

### Unreal Engine 5

Copy `integration/unreal/KaguraToolchain.cs` into your UBT toolchain path.
Obfuscation is applied automatically for Shipping builds.

See [`integration/unreal/`](integration/unreal/) for setup and `KaguraObfuscation.Build.cs`.

### Cocos2d-x / Custom CMake Engines

Use the generic toolchain file:

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/kagura/integration/cmake/kagura-toolchain.cmake \
      -DKAGURA_PLUGIN_PATH=/path/to/KaguraObfuscator.so \
      -DKAGURA_PROFILE=BALANCED \
      -B build -S .
```

Available profiles: `FAST`, `BALANCED` (default), `STRONG`, `OFF`.

Chain with an existing toolchain (e.g. Android NDK):

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=.../kagura-toolchain.cmake \
      -DKAGURA_CHAIN_TOOLCHAIN=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      ...
```

See [`integration/cmake/kagura-toolchain.cmake`](integration/cmake/kagura-toolchain.cmake).

## Runtime Library

Some passes require linking `libkagura_runtime.a` into the target binary:

| Pass / Feature | Runtime symbol |
|----------------|----------------|
| StringEncryptionAES | `kagura_aes_decrypt` |
| VMObfuscation | `kagura_vm_execute` |
| AntiDebug | `kagura_anti_debug_init`, `kagura_check_hooks`, `kagura_check_breakpoints`, `kagura_check_emulator` |
| AntiTamper | `kagura_self_check` |
| CallIndirection | `kagura_rtld_default_handle` |
| PointerAuth | `kagura_pac_key` |

```bash
clang your_file.c build/runtime/libkagura_runtime.a -o your_file
```

## Tests

```bash
cmake --build build --target check-kagura
# or
cd build && ctest --output-on-failure
```

## Pass Order Recommendation

Run kagura passes **after** standard optimizations to prevent LLVM from undoing obfuscation:

```
-O1 (or -O2)
  → kagura-sv          # hide symbols first
  → kagura-str[-aes]   # encrypt strings
  → kagura-genc        # encrypt globals
  → kagura-tamper      # integrity hash (before CFG changes)
  → kagura-ci          # external call indirection
  → kagura-pac         # pointer auth
  → kagura-fla         # CFG flattening
  → kagura-bcf         # bogus control flow
  → kagura-bbs         # BB splitting
  → kagura-bbr         # BB reordering
  → kagura-dci         # dead code insertion
  → kagura-sub         # instruction substitution
  → kagura-co          # constant obfuscation
  → kagura-anti-debug  # anti-analysis checks last
```

The plugin automatically hooks `registerOptimizerLastEPCallback` when using `-mllvm` flags, so the order above is applied automatically.

## License

MIT — see [LICENSE](LICENSE).
