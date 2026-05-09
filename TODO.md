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
