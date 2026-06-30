# Build from Source

=== "macOS (Homebrew LLVM)"

    ```bash
    brew install llvm
    bash build.sh
    ```

=== "macOS / Linux (Custom LLVM)"

    ```bash
    cmake -B build \
      -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
      -DCMAKE_C_COMPILER=/path/to/clang \
      -DCMAKE_CXX_COMPILER=/path/to/clang++ \
      .
    cmake --build build -j$(nproc)
    ```

=== "Windows (Clang-CL)"

    The LLVM dev tree is required for `LLVMConfig.cmake`. Kagura builds as a static
    library on Windows (`KaguraObfuscator.lib`) since MSVC does not support loadable
    pass modules — link the static lib directly into your driver tool.

    ```bat
    cmake -B build -G Ninja ^
      -DLLVM_DIR=C:\llvm-dev\lib\cmake\llvm ^
      -DCMAKE_C_COMPILER=C:\llvm-dev\bin\clang-cl.exe ^
      -DCMAKE_CXX_COMPILER=C:\llvm-dev\bin\clang-cl.exe ^
      -DKAGURA_BUILD_TESTS=ON
    cmake --build build
    ```

## Outputs

| Platform | Plugin path |
|:---------|:------------|
| macOS    | `build/lib/Transforms/KaguraObfuscator.dylib` |
| Linux    | `build/lib/Transforms/KaguraObfuscator.so` |
| Windows  | `build/lib/Transforms/KaguraObfuscator.lib` (static) |

Runtime library: `build/runtime/libkagura_runtime.a`

## Verifying the build

```bash
cd build && ctest --output-on-failure
```

See [Testing & Evaluation](../testing.md) for the full test matrix (differential, reproducible, angr / Ghidra / Frida).
