# 制御フロー難読化

ソース: `lib/Transforms/CFG/`

| フラグ | パス | 効果 |
|:-------|:-----|:-----|
| `-kagura-fla` | ControlFlowFlattening | CFG を switch ベースの状態機械に変換 (Wasm ではスキップ — 非構造化 CFG を要求するため) |
| `-kagura-bcf` | BogusControlFlow | MBA 不透明述語でガードされた死ブロックを注入 |
| `-kagura-ibr` | IndirectBranch | 直接呼び出しを関数ポインタグローバルからのロードに置換 |
| `-kagura-ci` | CallIndirection | 外部呼び出しを実行時解決のサンクテーブルへルーティング |
| `-kagura-lt` | LoopTransform | 偽の死カウンタと不透明な不変分岐を追加 |
| `-kagura-fsplit` | FunctionSplit | 内部 BB を抽出してアウトラインされたヘルパー関数にする |
| `-kagura-bbs` | BasicBlockSplitting | 大きな BB をランダムに分割し CFG 複雑度を増加 |
| `-kagura-bbr` | BasicBlockReordering | BB レイアウトをシャッフルし線形ディスアセンブラを混乱 |
| `-kagura-dci` | DeadCodeInsertion | 到達不能なジャンクブロックを挿入し静的解析を誤誘導 |
| `-kagura-elt` | EncryptedLookupTable | switch 文を XOR 暗号化されたディスパッチテーブルに変換 |
| `-kagura-vtp` | VTableProtection | C++ RTTI の typeinfo 名 (`_ZTS*`) を難読化、vtable メタデータを記録 |

変換後の IR の見え方は [Before / After 例](before-after.md) を参照。
