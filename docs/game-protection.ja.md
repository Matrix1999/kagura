# ゲーム保護

`include/kagura/game_protect.h` は C++17 ヘッダーオンリーの `Protected<T>` テンプレートを提供します。ゲームクリティカルな値 (HP, ダメージ, 通貨等) をメモリスキャナ (Cheat Engine, GameGuardian) や値フリーズツールから守ります。

これは [`kagura-mvo`](passes/data.md) と [`kagura-pe`](passes/data.md) のコンパイル時パスを**実行時**で補完するものです — 両方併用すると最大カバレッジになります。

## 使い方

```cpp
#include "kagura/game_protect.h"

kagura::Protected<int>   hp(100);
kagura::Protected<float> speed(5.5f);

hp -= 30;
if (hp <= 0) die();

// オプションのタンパーコールバック (デフォルト: 無限ループでクリーンなクラッシュ点を作らない)
kagura::Protected<int>::setTamperCallback([]{ report_cheat(); });
```

便利エイリアス: `ProtectedInt`, `ProtectedFloat`, `ProtectedDouble`, `ProtectedInt64` 等。

## 保護戦略

- 値は ASLR + スタックエントロピー由来の**インスタンスごとの実行時鍵**で XOR 暗号化して格納。各インスタンスが異なるマスクを持つ。
- **シャドウコピー**を別の XOR マスクで保持し、読み取りごとに外部書き込み (メモリエディタによる値書き換え) を検出。
- 平文値はレジスタ内にだけ存在し、メモリ上には**読み書きの間に常駐しない** — 演算子本体の実行中にのみ存在する。

## パスとの組み合わせ

| レイヤー | 防御対象 |
|:---------|:---------|
| `Protected<T>` (実行時) | メモリスキャナ、値フリーズツール、スクリプト書き換え |
| `kagura-mvo` (コンパイル時) | スタックフレームからの静的な値抽出 |
| `kagura-pe` (コンパイル時)  | ダンプされたヒープスナップショットでのポインタ追跡 |
| `kagura-anti-debug` (実行時) | デバッガアタッチ (Cheat Engine), Frida 注入 |
