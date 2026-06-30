# アンチ解析

ソース: `lib/Transforms/AntiAnalysis/`

| フラグ | パス | 効果 |
|:-------|:-----|:-----|
| `-kagura-anti-debug` | AntiDebug | ptrace, Frida ポート, `/proc/maps`, フック, ブレークポイント, エミュレータ検出 (iOS/Android)。IsDebuggerPresent, NtQueryInformationProcess, PEB ヒープフラグ (Windows)。Wasm ではスキップ |
| `-kagura-tamper` | AntiTamper | FNV-1a 関数チェックサム + 起動時の jailbreak / root 検出 |
| `-kagura-pac` | PointerAuth | XOR タグ付き関数ポインタグローバルによるソフトウェア CFI |
| `-kagura-sv` | SymbolVisibility | 非公開シンボルを hidden に設定、動的シンボルテーブルから削除 |
| `-kagura-honey` | HoneyValue | おとりのシークレットグローバルと偽のセキュリティスタブ関数を注入 |
| `-kagura-bbcheck` | BasicBlockChecksum | BB ごとのオペコードチェックサムを注入、バイナリパッチ検出時に abort |
| `-kagura-telemetry` | Telemetry | チート検出用に関数エントリへ振る舞いイベントプローブを挿入 |

ほとんどのアンチ解析パスは実行時に `libkagura_runtime.a` を呼び出します — シンボル対応表と直接呼び出し可能なチェック (`kagura_self_check`, `kagura_run_review_risk_check` 等) は [ランタイムライブラリ](../runtime.md) を参照。
