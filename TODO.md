# TODO — Unimplemented Items

Features that were not implemented in Phase 4 and are candidates for future work.

| ID | Feature | Priority | Notes |
|:---|:--------|:---------|:------|
| 4.2.11 | White-box cryptography | Low | AES/TDES white-box key encoding so the cipher key is never exposed in plaintext even under full memory disclosure |
| 4.4.19 | libil2cpp.so / libUE4.so specialized protection | Med | Tailored integrity checks and obfuscation patterns for IL2CPP and UE4 native libraries |
| 4.7.9 | Device farm tests (iOS/Android real devices) | Low | Run the integration test suite on physical iOS and Android devices via a cloud device farm (e.g. Firebase Test Lab, AWS Device Farm) |
| 4.8.1 | Obfuscation transform auto-selection (risk-based) | Med | Analyze the IR and automatically select the most cost-effective pass combination based on function complexity and attack surface |
| 4.8.9 | ML/heuristic-based protection target inference | Low | Use static features (cyclomatic complexity, symbol names, call graph centrality) to recommend which functions to protect |
| 4.8.10 | Machine code / backend-level obfuscation | Low | Apply obfuscation at the assembler / MachineFunction level (after instruction selection) for deeper resistance to IR-level static analysis |
| — | MSVC ABI support in VTableProtection | Med | Handle `??_7` / `??_R` MSVC naming conventions in addition to Itanium `_ZTV`/`_ZTS`/`_ZTI`; required for Windows target support |
| — | Partial pass application for EH functions | Med | BBR, PE, and MVO currently skip entire functions that contain exception handling; apply passes to non-EH blocks only instead of bailing out |
| — | VMObfuscation: widen jump offset to uint32 | Low | Bytecode jump targets are capped at uint16_t (64 KB); large functions silently get NOP'd. Widening to uint32 removes this hard limit |
| — | Regression corpus expansion | High | Only one entry in tests/regression/corpus/; add .ll test cases covering EH, PHI-heavy, vararg, empty, and large functions for each pass |
| — | FunctionSplit: allow call-containing blocks | Low | Blocks with simple leaf calls (no indirect, no EH) are unconditionally skipped; selectively allow noinline-safe calls to increase extraction rate |
