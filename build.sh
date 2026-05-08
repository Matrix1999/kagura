#!/usr/bin/env bash
set -euo pipefail

LLVM_PREFIX=/opt/homebrew/opt/llvm
BUILD_DIR=${1:-build}

cmake -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_PREFIX_PATH="$LLVM_PREFIX" \
  -DLLVM_DIR="$LLVM_PREFIX/lib/cmake/llvm" \
  -DCMAKE_C_COMPILER="$LLVM_PREFIX/bin/clang" \
  -DCMAKE_CXX_COMPILER="$LLVM_PREFIX/bin/clang++" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  .

cmake --build "$BUILD_DIR" -- -j"$(sysctl -n hw.logicalcpu)"
echo ""
echo "Plugin: $BUILD_DIR/lib/Transforms/KaguraObfuscator.dylib"
echo ""
echo "Usage:"
echo "  clang -fpass-plugin=$BUILD_DIR/lib/Transforms/KaguraObfuscator.dylib \\"
echo "        -mllvm -kagura-str -mllvm -kagura-fla -mllvm -kagura-bcf \\"
echo "        -O1 your_file.c -o your_file"
