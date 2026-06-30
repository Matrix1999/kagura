# ソースからビルド

=== "macOS (Homebrew LLVM)"

    ```bash
    brew install llvm
    bash build.sh
    ```

=== "macOS / Linux (カスタム LLVM)"

    ```bash
    cmake -B build \
      -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
      -DCMAKE_C_COMPILER=/path/to/clang \
      -DCMAKE_CXX_COMPILER=/path/to/clang++ \
      .
    cmake --build build -j$(nproc)
    ```

=== "Windows (Clang-CL)"

    `LLVMConfig.cmake` が必要なので、LLVM 開発ツリーが必要です。Windows では Kagura は静的ライブラリ (`KaguraObfuscator.lib`) としてビルドされます。MSVC はロード可能なパスモジュールに対応していないため、静的ライブラリをドライバツールに直接リンクしてください。

    ```bat
    cmake -B build -G Ninja ^
      -DLLVM_DIR=C:\llvm-dev\lib\cmake\llvm ^
      -DCMAKE_C_COMPILER=C:\llvm-dev\bin\clang-cl.exe ^
      -DCMAKE_CXX_COMPILER=C:\llvm-dev\bin\clang-cl.exe ^
      -DKAGURA_BUILD_TESTS=ON
    cmake --build build
    ```

## 出力先

| プラットフォーム | プラグインパス |
|:-----------------|:--------------|
| macOS    | `build/lib/Transforms/KaguraObfuscator.dylib` |
| Linux    | `build/lib/Transforms/KaguraObfuscator.so` |
| Windows  | `build/lib/Transforms/KaguraObfuscator.lib` (静的) |

ランタイムライブラリ: `build/runtime/libkagura_runtime.a`

## ビルドの検証

```bash
cd build && ctest --output-on-failure
```

テスト一覧 (differential, reproducible, angr / Ghidra / Frida) は [テスト・評価](../testing.md) を参照。
