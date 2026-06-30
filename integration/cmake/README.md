# kagura — CMake Toolchain Integration

A generic CMake toolchain file that wires the KaguraObfuscator plugin and
runtime library into any CMake project (Cocos2d-x, Godot GDNative, custom
engines, …). It also **chains cleanly** with platform toolchains like the
Android NDK toolchain.

## Files

| File | Purpose |
|:-----|:--------|
| `kagura-toolchain.cmake` | Toolchain file — injects `-fpass-plugin=...` and `-mllvm -kagura-*` into `CMAKE_{C,CXX}_FLAGS_INIT`, and the runtime archive into `CMAKE_*_LINKER_FLAGS_INIT` |

---

## Quick start

```bash
cmake \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/kagura/integration/cmake/kagura-toolchain.cmake \
  -DKAGURA_PLUGIN_PATH=/path/to/KaguraObfuscator.dylib \
  -B build -S .
cmake --build build
```

## Chaining with another toolchain (e.g. Android NDK)

```bash
cmake \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/kagura/integration/cmake/kagura-toolchain.cmake \
  -DKAGURA_CHAIN_TOOLCHAIN=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DKAGURA_PLUGIN_PATH=/path/to/KaguraObfuscator.so \
  -B build -S .
```

Set `KAGURA_CHAIN_TOOLCHAIN` to the **inner** toolchain. Kagura's toolchain
includes it first, then layers the obfuscation flags on top.

---

## Variables

### Required

| Variable | Description |
|:---------|:------------|
| `KAGURA_PLUGIN_PATH` | Path to `KaguraObfuscator.dylib` / `.so` |

### Optional

| Variable | Default | Description |
|:---------|:--------|:------------|
| `KAGURA_RUNTIME_LIB`     | auto-detect | Path to `libkagura_runtime.a` |
| `KAGURA_CHAIN_TOOLCHAIN` | —           | Another toolchain file to include first |
| `KAGURA_PROFILE`         | `BALANCED`  | `FAST` / `BALANCED` / `STRONG` / `OFF` |
| `KAGURA_SEED`            | `0`         | PRNG seed |
| `KAGURA_BCF_PROB`        | `30`        | Bogus CF probability |

---

## Profiles

| Profile | Passes enabled |
|:--------|:---------------|
| `FAST`     | str, sv, anti-debug |
| `BALANCED` | str, fla, bcf, sub, ibr, bbr, sv, anti-debug, tamper *(default)* |
| `STRONG`   | All `BALANCED` passes + co, genc, bbs, dci, vm |
| `OFF`      | No obfuscation (toolchain still chains correctly) |

See [Configuration](https://ykus4.github.io/kagura/configuration/) for finer-grained control via
a JSON policy file.

---

## Behaviour notes

- The toolchain emits `[kagura] Toolchain: profile=...`, `Plugin: ...`,
  `Runtime: ...` at configure time so you can verify the wiring.
- If `KAGURA_PLUGIN_PATH` is unset, the toolchain tries
  `${CMAKE_CURRENT_LIST_DIR}/../../build/lib/Transforms/KaguraObfuscator.{dylib,so}`
  before warning and disabling obfuscation.
- Flags go into the `_INIT` variants of `CMAKE_*_FLAGS`, so they take effect
  before `project()` and propagate to sub-projects via cache.
- Setting `KAGURA_PROFILE=OFF` makes the toolchain a no-op (still chains the
  inner toolchain). Use this for debug builds.
