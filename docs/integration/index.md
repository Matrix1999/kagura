# Integration

Quick-setup matrix for every supported build system:

| Platform | Quick setup |
|:---------|:------------|
| [**Xcode**](xcode.md) | Add `integration/xcode/kagura.xcconfig` + run-script phase |
| [**Android (Gradle / NDK)**](android.md) | `apply from: "kagura/integration/android/kagura.gradle"` |
| [**Unity (IL2CPP)**](unity.md) | Copy `Editor/KaguraPostBuildProcessor.cs` to `Assets/Editor/` |
| [**Unreal Engine 5**](unreal.md) | Copy `KaguraToolchain.cs` to the UBT toolchain path |
| [**CMake**](cmake.md) | `-DCMAKE_TOOLCHAIN_FILE=kagura-toolchain.cmake` |
| [**Bazel**](bazel.md) | `load("@kagura//integration/bazel:kagura.bzl", "kagura_cc_binary")` |
| [**CocoaPods**](cocoapods.md) | `pod 'KaguraObfuscator'` + xcconfig for the plugin flag |
| [**Swift Package Manager**](swiftpm.md) | `.product(name: "KaguraRuntime", package: "kagura")` |

Each page mirrors the README that ships alongside the integration files in the
repo (`integration/<system>/README.md`), so what you see here is the same as
what you read on GitHub.
