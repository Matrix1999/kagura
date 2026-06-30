# CMake ツールチェイン統合

詳細は GitHub の [`integration/cmake/README.md`](https://github.com/ykus4/kagura/blob/main/integration/cmake/README.md) (英語) を参照してください。

> ⚠️ 日本語訳は今後追加予定です。最新かつ完全な手順は上記英語版にあります。

## クイックスタート

```bash
cmake \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/kagura/integration/cmake/kagura-toolchain.cmake \
  -DKAGURA_PLUGIN_PATH=/path/to/KaguraObfuscator.dylib \
  -B build -S .
cmake --build build
```

## プロファイル

| プロファイル | パス |
|:-------------|:-----|
| `FAST`     | str, sv, anti-debug |
| `BALANCED` | str, fla, bcf, sub, ibr, bbr, sv, anti-debug, tamper *(デフォルト)* |
| `STRONG`   | `BALANCED` 全部 + co, genc, bbs, dci, vm |
| `OFF`      | 難読化なし (ツールチェインのチェーンは動作) |

詳細は英語版 README を、関連項目は [統合](index.md) を参照してください。
