# インフラパス

ソース: `lib/Transforms/Infrastructure/`

| フラグ | パス | 効果 |
|:-------|:-----|:-----|
| `-kagura-dwarf=strip\|obfuscate` | DWARFControl | 難読化後の DWARF デバッグ情報を strip または再マップ |
| `-kagura-config=<file>` | ConfigLoader | JSON ポリシーファイルを読み込み、プロファイルプリセットと per-pass オーバーライドを適用 |
| `-kagura-symmap` | SymbolMap | クラッシュシンボル化のための JSON シンボルマップ (元名 → 難読化名) を出力 |
| `-kagura-audit` | AuditLog | 保護対象シンボルと適用パスの JSON 監査ログを出力 |

## ユーティリティ

| フラグ | パス | 効果 |
|:-------|:-----|:-----|
| `-kagura-metrics` | ObfuscationMetrics | BB / 命令 / 循環的複雑度の差分を表示 |

`-kagura-config` が受け取る JSON DSL は [設定](../configuration.md) を、関連する `-kagura-symmap-out`、`-kagura-audit-out`、シンボルフィルタは [チューニングパラメータ](../tuning.md) を参照。
