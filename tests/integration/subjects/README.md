# tests/integration/subjects/

C source files used as subjects for **integration tests**
(`tests/integration/CMakeLists.txt`).

Each file is:
1. Compiled to a native binary **without** kagura (baseline)
2. Compiled to a native binary **with** kagura passes enabled
3. Both binaries are executed; their stdout must be identical

These files must produce **stable, deterministic output** and are
linked without any special kagura runtime (no AES decryption, no
VM interpreter).  Keep them minimal.

See `tests/pass-inputs/` for bitcode-level inputs that exercise
individual passes without requiring a runnable binary.
