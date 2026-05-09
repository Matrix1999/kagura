# kagura — Android NDK Integration

This directory contains the Android NDK CMake integration for the kagura LLVM
obfuscator.  It provides helper functions that wire the obfuscation plugin and
its companion runtime library into an Android Studio / Gradle project with
minimal boilerplate.

---

## Files

| File | Purpose |
|---|---|
| `kagura-android-ndk.cmake` | CMake helper — defines `kagura_android_target()`, `kagura_android_config()`, `kagura_android_runtime_target()` |
| `kagura-cmake.cmake` | Lean single-function include for projects that only need `kagura_target()` |
| `kagura.gradle` | Groovy Gradle script that injects flags into `externalNativeBuild` |

---

## Android Studio Integration

### Step 1 — Build the plugin

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# produces build/lib/Transforms/KaguraObfuscator.so
```

### Step 2 — Add to `app/build.gradle`

```groovy
android {
    defaultConfig {
        externalNativeBuild {
            cmake {
                arguments "-DKAGURA_PLUGIN_PATH=${rootDir}/../kagura/build/lib/Transforms/KaguraObfuscator.so",
                          "-DKAGURA_PROFILE=BALANCED"
            }
        }
    }
    externalNativeBuild {
        cmake { path "src/main/cpp/CMakeLists.txt" }
    }
}
```

### Step 3 — Include in `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.22)
project(mygame)

include(${CMAKE_SOURCE_DIR}/../kagura/integration/android/kagura-android-ndk.cmake)

kagura_android_config()

add_library(mynativelib SHARED src/native.cpp)
kagura_android_target(mynativelib)
```

To also compile the kagura runtime (anti-debug, AES decrypt stubs, IL2CPP
protection) into your build, add one line before `kagura_android_target`:

```cmake
kagura_android_runtime_target(kagura_runtime)
kagura_android_target(mynativelib)   # auto-links kagura_runtime
```

---

## CMake Flags Reference

| Variable | Type | Default | Description |
|---|---|---|---|
| `KAGURA_PLUGIN_PATH` | PATH | auto | Absolute path to `KaguraObfuscator.so` |
| `KAGURA_PROFILE` | STRING | `BALANCED` | Preset profile (see below) |
| `KAGURA_RUNTIME_DIR` | PATH | auto | Directory with runtime `.c` sources |
| `KAGURA_ENABLE_STR` | BOOL | ON | Narrow string encryption |
| `KAGURA_ENABLE_STR_AES` | BOOL | OFF | AES-128-CTR string encryption |
| `KAGURA_ENABLE_WSTR` | BOOL | ON | Wide string encryption |
| `KAGURA_ENABLE_FLA` | BOOL | ON | CFG flattening |
| `KAGURA_ENABLE_BCF` | BOOL | OFF | Bogus control flow |
| `KAGURA_ENABLE_SUB` | BOOL | OFF | Instruction substitution |
| `KAGURA_ENABLE_CO` | BOOL | OFF | Constant obfuscation (MBA) |
| `KAGURA_ENABLE_GENC` | BOOL | OFF | Global integer encryption |
| `KAGURA_ENABLE_MVO` | BOOL | OFF | Memory value obfuscation (local integer XOR) |
| `KAGURA_ENABLE_JNI` | BOOL | ON | JNI dynamic registration |
| `KAGURA_ENABLE_ANTIDEBUG` | BOOL | ON | Anti-debug / Anti-Frida |
| `KAGURA_ENABLE_TAMPER` | BOOL | ON | Anti-tamper integrity checks |
| `KAGURA_ENABLE_HONEY` | BOOL | OFF | Honey value / fake symbol injection |
| `KAGURA_ENABLE_IL2CPP` | BOOL | OFF | IL2CPP runtime protection |
| `KAGURA_BCF_PROB` | STRING | 30 | Bogus CF probability 0–100 |
| `KAGURA_BCF_ITER` | STRING | 1 | Bogus CF iterations |
| `KAGURA_SUB_ITER` | STRING | 1 | Instruction substitution iterations |
| `KAGURA_SEED` | STRING | 0 | PRNG seed (0 = system entropy) |
| `KAGURA_METRICS` | BOOL | OFF | Print obfuscation metrics to stdout |
| `KAGURA_SYMMAP` | BOOL | OFF | Emit JSON symbol map |

Fine-grained toggles are only applied when `KAGURA_PROFILE=CUSTOM`.  All other
profiles override the individual flags.

---

## Profile Presets

| Profile | Passes enabled | BCF prob | Intended use |
|---|---|---|---|
| `FAST` | str, jni, anti-debug | — | Hot paths, CI builds, debug variants |
| `BALANCED` | str, wstr, bcf, bbr, bbs, genc, mvo, jni, anti-debug, tamper | 30 | Release builds (default) |
| `STRONG` | all passes + il2cpp | 60, 2 iter | Security-critical shipping builds |
| `CUSTOM` | whatever `KAGURA_ENABLE_*` says | user-defined | Fine-grained control |

Profiles can also be set via the JSON config DSL:

```cmake
set(KAGURA_CONFIG_PATH "${CMAKE_SOURCE_DIR}/kagura.json")
kagura_android_config()
```

```json
{ "profile": "STRONG" }
```

---

## Gradle Plugin Usage

For projects that prefer to configure everything from Gradle, apply the
companion script instead of using CMake arguments directly:

```groovy
// app/build.gradle
apply from: "${rootDir}/../kagura/integration/android/kagura.gradle"
```

Override individual settings before the `apply from` line:

```groovy
ext.kagura = [
    pluginPath : "/opt/kagura/build/lib/Transforms/KaguraObfuscator.so",
    enableBcf  : true,
    bcfProb    : 40,
    enableCo   : false,
]
apply from: "${rootDir}/../kagura/integration/android/kagura.gradle"
```

Settings can also be placed in `local.properties` (not committed to VCS):

```properties
kagura.pluginPath=/Users/me/kagura/build/lib/Transforms/KaguraObfuscator.so
```

---

## ABI Notes

| ABI | Notes |
|---|---|
| `arm64-v8a` | Fully supported; all passes tested. Recommended primary target. |
| `armeabi-v7a` | Supported. BCF iterations automatically capped at 1 to limit code-size growth on Thumb-2. |
| `x86_64` | Supported (emulator / Chrome OS). BCF adds branch-prediction overhead; keep `KAGURA_BCF_PROB` at or below 20. |
| `x86` | Compiles but BCF is discouraged — 32-bit x86 Android is effectively end-of-life. |

---

## Performance Impact (Rough Estimates)

These figures are measured on a mid-range Arm Cortex-A55 device running
Android 12.  Actual impact depends heavily on code structure.

| Profile | Binary size increase | CPU overhead (hot loop) | Build time increase |
|---|---|---|---|
| FAST | +5 – 10 % | < 2 % | +10 – 20 % |
| BALANCED | +15 – 25 % | 3 – 8 % | +25 – 40 % |
| STRONG | +40 – 70 % | 10 – 20 % | +60 – 100 % |

Startup time is unaffected by the obfuscation passes themselves; any
measurable startup delta comes from the runtime self-check
(`kagura_self_check`) which typically completes in under 5 ms.

String decryption stubs add a one-time per-string decryption cost on first
use.  Subsequent accesses hit the decrypted copy in `.data` without overhead.
