# tests/pass-inputs/

C source files used as inputs for **pass-level tests** (`tests/CMakeLists.txt`).

Each file is:
1. Compiled to LLVM bitcode (`clang -O0 -emit-llvm`)
2. Passed through `opt --load-pass-plugin` with a specific kagura pass
3. Checked for crash-free execution (no semantic output comparison)

These files may contain kagura annotation attributes and are **not** expected
to produce stable output as standalone binaries — they exist only to exercise
the pass transforms on realistic IR patterns.

See `tests/integration/subjects/` for executable smoke tests that verify
semantic correctness (obfuscated output must match plain output).
