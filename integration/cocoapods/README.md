# kagura — CocoaPods Integration

A `podspec` that vendors the Kagura **runtime library** into an iOS / macOS
app via CocoaPods, and adds a build-phase script that loads the obfuscator
plugin.

## Files

| File | Purpose |
|:-----|:--------|
| `kagura.podspec` | Pod spec for `KaguraObfuscator` — compiles `runtime/**/*.c` into your target and registers a `before_compile` script phase |

---

## Usage

In your `Podfile`:

```ruby
target 'MyApp' do
  pod 'KaguraObfuscator',
      :git    => 'https://github.com/ykus4/kagura.git',
      :tag    => 'v0.1.0'
end
```

```bash
pod install
```

CocoaPods will:

1. Vendor the runtime sources into your workspace as a static library target.
2. Install a `before_compile` script phase that injects the plugin into
   `OTHER_CFLAGS` if it finds `${PODS_ROOT}/KaguraObfuscator/build/lib/libKaguraObfuscator.dylib`.

You still need to **build the plugin** yourself before opening Xcode:

```bash
cd Pods/KaguraObfuscator
cmake -B build -DLLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm
cmake --build build
```

After that, an ordinary `xcodebuild` (or pressing ⌘B in Xcode) will pick the
plugin up automatically.

---

## What's vendored

The podspec includes everything under `runtime/**/*.{c,h}` and `include/**/*.h`
**except** Android / Linux-only sources, which are excluded so they don't
break the iOS / macOS build:

```
runtime/jni_hook_detection.c     (Android-only)
runtime/play_integrity.c         (Android-only)
runtime/safetynet_compat.c       (Android-only)
runtime/art_environment.c        (Android-only)
runtime/proc_inspection.c        (Linux / Android-only)
runtime/seccomp_checks.c         (Linux / Android-only)
runtime/load_order.c             (Linux / Android-only)
runtime/direct_syscall.c         (platform-specific, guarded internally)
```

---

## Compiler settings

| Setting | Value |
|:--------|:------|
| `compiler_flags`           | `-std=c11` |
| `GCC_OPTIMIZATION_LEVEL`   | `2` |
| `HEADER_SEARCH_PATHS`      | `$(PODS_TARGET_SRCROOT)/include` |
| Platforms                  | iOS 13+, macOS 11+ |

For richer Xcode-side configuration (per-target xcconfig, per-file selective
obfuscation, code-signing notes), see [Xcode Integration](https://ykus4.github.io/kagura/integration/xcode/).
