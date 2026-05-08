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
| `-kagura-lt` | LoopTransform | Adds bogus dead counters and opaque invariant branches to loops |
| `-kagura-fsplit` | FunctionSplit | Extracts interior basic blocks into outlined helper functions |
| `-kagura-vm` | VMObfuscation | Virtualizes function bodies into a custom stack-based VM bytecode |

### Data

| Flag | Pass | Description |
|------|------|-------------|
| `-kagura-str` | StringEncryption | XOR-encrypts string literals at compile time |
| `-kagura-str-aes` | StringEncryptionAES | AES-128-CTR string encryption (requires `kagura_runtime`) |
| `-kagura-co` | ConstantObfuscation | Replaces integer constants with MBA expressions |
| `-kagura-sub` | Substitution | Replaces arithmetic/bitwise ops with equivalent MBA expressions |

### Anti-Analysis

| Flag | Pass | Description |
|------|------|-------------|
| `-kagura-anti-debug` | AntiDebug | Injects ptrace, port 27042, and `/proc/maps` checks |
| `-kagura-tamper` | AntiTamper | Injects FNV-1a function checksums and jailbreak/root detection |

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

## Runtime Library

Some passes require linking `libkagura_runtime.a` into the target binary:

| Pass | Requires runtime |
|------|-----------------|
| StringEncryptionAES | yes (`kagura_aes_decrypt`) |
| VMObfuscation | yes (`kagura_vm_execute`) |
| AntiDebug | yes (`kagura_anti_debug_init`) |
| AntiTamper | yes (`kagura_self_check`) |

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
-O1 (or -O2) → kagura-str → kagura-tamper → kagura-fla → kagura-bcf → kagura-sub → kagura-co
```

The plugin automatically hooks `registerOptimizerLastEPCallback` when using `-mllvm` flags, so the order above is applied automatically.

## License

MIT — see [LICENSE](LICENSE).
