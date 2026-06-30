# Anti-Analysis

Source: `lib/Transforms/AntiAnalysis/`

| Flag | Pass | Effect |
|:-----|:-----|:-------|
| `-kagura-anti-debug` | AntiDebug | ptrace, Frida port, `/proc/maps`, hook, breakpoint, emulator checks (iOS/Android); IsDebuggerPresent, NtQueryInformationProcess, PEB heap flags (Windows); skipped on Wasm |
| `-kagura-tamper` | AntiTamper | FNV-1a function checksums + jailbreak/root detection at startup |
| `-kagura-pac` | PointerAuth | Software CFI via XOR-tagged function pointer globals |
| `-kagura-sv` | SymbolVisibility | Sets non-public symbols to hidden; strips from dynamic symtab |
| `-kagura-honey` | HoneyValue | Injects decoy secret globals and fake security-stub functions |
| `-kagura-bbcheck` | BasicBlockChecksum | Injects per-BB opcode checksums; aborts on binary patch detection |
| `-kagura-telemetry` | Telemetry | Inserts behavioral event probes at function entry for cheat detection |

Most anti-analysis passes call into `libkagura_runtime.a` at run time — see
[Runtime Library](../runtime.md) for the symbol matrix and a list of directly
callable checks (e.g. `kagura_self_check`, `kagura_run_review_risk_check`).
