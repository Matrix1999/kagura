# 統合

すべての対応ビルドシステムのクイックセットアップマトリクス:

| プラットフォーム | クイックセットアップ |
|:---------------|:--------------------|
| [**Xcode**](xcode.md) | `integration/xcode/kagura.xcconfig` を追加 + run-script フェーズ |
| [**Android (Gradle / NDK)**](android.md) | `apply from: "kagura/integration/android/kagura.gradle"` |
| [**Unity (IL2CPP)**](unity.md) | `Editor/KaguraPostBuildProcessor.cs` を `Assets/Editor/` にコピー |
| [**Unreal Engine 5**](unreal.md) | `KaguraToolchain.cs` を UBT toolchain パスにコピー |
| [**CMake**](cmake.md) | `-DCMAKE_TOOLCHAIN_FILE=kagura-toolchain.cmake` |
| [**Bazel**](bazel.md) | `load("@kagura//integration/bazel:kagura.bzl", "kagura_cc_binary")` |
| [**CocoaPods**](cocoapods.md) | `pod 'KaguraObfuscator'` + プラグインフラグの xcconfig |
| [**Swift Package Manager**](swiftpm.md) | `.product(name: "KaguraRuntime", package: "kagura")` |

各ページは統合ファイル群とともに同梱される README (`integration/<system>/README.md`) と同等の情報を提供します。

> **注**: 各統合ガイドの本文は GitHub 上の `integration/<system>/README.md` (英語) を参照してください。日本語訳は今後追加予定です。
