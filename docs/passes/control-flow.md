# Control Flow Obfuscation

Source: `lib/Transforms/CFG/`

| Flag | Pass | Effect |
|:-----|:-----|:-------|
| `-kagura-fla` | ControlFlowFlattening | Converts CFG into a switch-based state machine (skipped on Wasm — requires unstructured CFG) |
| `-kagura-bcf` | BogusControlFlow | Injects dead blocks guarded by MBA opaque predicates |
| `-kagura-ibr` | IndirectBranch | Replaces direct calls with loads from function pointer globals |
| `-kagura-ci` | CallIndirection | Routes external calls through a runtime-resolved thunk table |
| `-kagura-lt` | LoopTransform | Adds bogus dead counters and opaque invariant branches |
| `-kagura-fsplit` | FunctionSplit | Extracts interior basic blocks into outlined helper functions |
| `-kagura-bbs` | BasicBlockSplitting | Splits large BBs at random points to inflate CFG complexity |
| `-kagura-bbr` | BasicBlockReordering | Shuffles BB layout to confuse linear disassemblers |
| `-kagura-dci` | DeadCodeInsertion | Inserts unreachable junk blocks to mislead static analysis |
| `-kagura-elt` | EncryptedLookupTable | Transforms switch statements into XOR-encrypted dispatch tables |
| `-kagura-vtp` | VTableProtection | Obfuscates C++ RTTI typeinfo names (`_ZTS*`); records vtable metadata |

See [Before / After Examples](before-after.md) for what the resulting IR looks like.
