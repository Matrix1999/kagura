# 設定

Kagura は `-kagura-config=<path>` で JSON ポリシーファイルを受け取り、すべてのパス設定を一箇所で制御できます。実プロジェクトで難読化を運用する推奨方法です。

## JSON DSL

```json
{
  "profile": "BALANCED",
  "passes": {
    "str":   true,
    "fla":   true,
    "bcf":   true,
    "honey": true,
    "mvo":   false
  },
  "tuning": {
    "bcf_prob": 40,
    "seed":     12345
  }
}
```

ポリシーファイルはパイプライン先頭の **`kagura-config`** パスで読み込まれ、後続パスのデフォルト値を設定します。関数単位の [`__attribute__((annotate("kagura_*")))`](getting-started/quick-start.md#5) オーバーライドは関数ごとに優先されます。

## 強度プロファイル

`"profile"` キーで選択する組み込みプロファイル:

| プロファイル | パス | 用途 |
|:-------------|:-----|:-----|
| `FAST`     | STR のみ | デバッグ / CI ビルド、最小オーバーヘッド |
| `BALANCED` | STR + BCF + BBR + BBS + GENC + MVO | 標準リリースビルド |
| `STRONG`   | 全パス、BCF 確率60、2 イテレーション | セキュリティクリティカルな出荷ビルド |

プロファイルはデフォルトを設定するだけ。`"passes"` や `"tuning"` で個別キーをオーバーライドできます。

## 実例 — 銀行 / FinTech リリース

STRONG プロファイル + per-build AES 鍵ローテーションで、あるバージョンから抽出された鍵が次バージョンで無効になるようにします:

```json title="kagura-bank.json"
{
  "profile": "STRONG",
  "passes": {
    "str-aes":  true,
    "mvo":      true,
    "pe":       true,
    "bbcheck":  true,
    "tamper":   true
  },
  "tuning": {
    "bcf_prob": 60,
    "seed":     0
  }
}
```

```bash
clang -fpass-plugin=KaguraObfuscator.dylib \
      -mllvm -kagura-config=kagura-bank.json \
      -mllvm -kagura-build-id=$(git rev-parse HEAD) \
      -O2 -c bank_core.c -o bank_core.o
```

## 関連

- [チューニングパラメータ](tuning.md) — すべての CLI フラグ、シンボルフィルタと `-kagura-build-id` per-build 鍵シードを含む。
- [パス順序](pass-order.md) — プラグインがこれらのパスを適用する決定論的な順序。
- [ゲーム保護](game-protection.md) — 実行時値保護のための `Protected<T>` (`mvo` / `pe` を補完)。
