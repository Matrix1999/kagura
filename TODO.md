# TODO — Unimplemented Items

Features that were not implemented in Phase 4 and are candidates for future work.

| ID | Feature | Priority | Notes |
|:---|:--------|:---------|:------|
| 4.2.11 | White-box cryptography | Low | AES/TDES white-box key encoding so the cipher key is never exposed in plaintext even under full memory disclosure |
| 4.7.9 | Device farm tests (iOS/Android real devices) | Low | Run the integration test suite on physical iOS and Android devices via a cloud device farm (e.g. Firebase Test Lab, AWS Device Farm) |
| 4.8.9 | ML/heuristic-based protection target inference | Low | Use static features (cyclomatic complexity, symbol names, call graph centrality) to recommend which functions to protect |
| 4.8.10 | Machine code / backend-level obfuscation | Low | Apply obfuscation at the assembler / MachineFunction level (after instruction selection) for deeper resistance to IR-level static analysis |
| — | MSVC ABI support in VTableProtection | Med | Handle `??_7` / `??_R` MSVC naming conventions in addition to Itanium `_ZTV`/`_ZTS`/`_ZTI`; required for Windows target support |
| — | Regression corpus expansion | High | Only one entry in tests/regression/corpus/; add .ll test cases covering EH, PHI-heavy, vararg, empty, and large functions for each pass |
| A.1 | Windows/MSVC build support | High | CMake skeleton exists but MSVC ABI IR is untested end-to-end; required for enterprise Windows targets |
| A.2 | Swift / Kotlin Native string encryption | High | StringEncryption currently skips Swift and Kotlin Native constant strings on iOS/Android |
| ~~A.3~~ | ~~BCF opaque predicate diversification~~ | ~~High~~ | ~~Implemented: 7 variants (OR/XOR/ADD complement, consecutive-int products, OR-odd, bit round-trip), selected at random per injection site~~ |
| B.1 | Decompiler resistance scoring | Medium | Automate Binary Ninja / Ghidra analysis to produce a quantitative resilience score; feed back into AutoSelectPass |
| B.2 | CallIndirection hot-path cache | Medium | Resolution happens on every call; a per-site cache would reduce overhead on hot paths |
| B.3 | Deterministic obfuscation sessions | Medium | Given the same seed key and IR, produce a bit-identical output binary to ensure CI reproducibility |
| C.1 | WebAssembly backend support | Low | Unity WebGL and other Wasm targets are growing; needs a Wasm-aware pass pipeline |
| C.2 | Automated MBA expression generation | Low | Current SUB uses fixed patterns; auto-generate Z3-verified equivalent expressions for richer MBA substitution |
