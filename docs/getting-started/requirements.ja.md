# 動作要件

| 要件 | バージョン |
|:-----|:----------|
| LLVM        | 17 – 22 (17, 18, 19, 21, 22 でテスト済) |
| CMake       | 3.20+ |
| C++         | C++17 |

## プラットフォームに関する注意

- **Windows** — Clang-CL のみ対応。MSVC はロード可能なパスモジュールに非対応なため、プラグインは静的ライブラリ (`KaguraObfuscator.lib`) としてビルドされます。ドライバツールに直接リンクしてください。

- **WebAssembly** — `kagura-fla` と `kagura-anti-debug` はスキップされます。Wasm は構造化制御フローを要求し、ネイティブのデバッガ面を持たないためです。

- **iOS / Android** — ビルドシステムへの組み込み方は [統合](../integration/index.md) を参照 (Xcode, Gradle / NDK)。
