# Xcode 統合

詳細な手順は GitHub の [`integration/xcode/README.md`](https://github.com/ykus4/kagura/blob/main/integration/xcode/README.md) (英語) を参照してください。

> ⚠️ このページの日本語訳は今後追加予定です。最新かつ完全な手順は上記英語版にあります。

## 概要

- `integration/xcode/kagura.xcconfig` をプロジェクトに追加
- `integration/xcode/run-kagura.sh` を Run Script Build Phase として登録
- プラグインは `KAGURA_PLUGIN_PATH` 環境変数または xcconfig 内 `KAGURA_PLUGIN_PATH = $(SRCROOT)/.../KaguraObfuscator.dylib` で指定

詳細は英語版 README を、関連項目は [統合](index.md) を参照してください。
