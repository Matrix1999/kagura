# Infrastructure Passes

Source: `lib/Transforms/Infrastructure/`

| Flag | Pass | Effect |
|:-----|:-----|:-------|
| `-kagura-dwarf=strip\|obfuscate` | DWARFControl | Strip or remap DWARF debug info after obfuscation |
| `-kagura-config=<file>` | ConfigLoader | Load JSON policy file; apply profile preset and per-pass overrides |
| `-kagura-symmap` | SymbolMap | Emit JSON symbol map (original → obfuscated name) for crash symbolication |
| `-kagura-audit` | AuditLog | Emit JSON audit log of every protected symbol and applied passes |

## Utilities

| Flag | Pass | Effect |
|:-----|:-----|:-------|
| `-kagura-metrics` | ObfuscationMetrics | Prints BB / instruction / cyclomatic complexity delta |

See [Configuration](../configuration.md) for the JSON DSL accepted by
`-kagura-config`, and [Tuning Parameters](../tuning.md) for the related
`-kagura-symmap-out`, `-kagura-audit-out`, and symbol filter options.
