# Before / After 例

攻撃者から実際にどう見えるか、Kagura 適用前と適用後。

---

## 文字列暗号化 (`-kagura-str`)

**Before** — 平文の `.rodata` 内の文字列リテラル:

```llvm
@api_key = private constant [33 x i8] c"sk-prod-9f2a1c3e8b4d7f0e1a2c3d4e5f6a7b8c\00"

define void @connect() {
  call void @send_auth(ptr @api_key)
}
```

**After** — XOR 暗号化されたブロブ。初回呼び出しで復号、直後にゼロクリア:

```llvm
@api_key.enc = private constant [33 x i8] c"\xde\xad\x7f\x12..."  ; 暗号化済
@api_key.dec = global [33 x i8] zeroinitializer                    ; 平文は短時間だけここに存在

define void @connect() {
  ; 注入された復号スタブ — フラグ確認、XOR で .dec に展開、send_auth 呼び出し、.dec をゼロ化
  call void @__kagura_str_init_0()
  call void @send_auth(ptr @api_key.dec)
}
```

`strings` を実行してもガベージしか返らない。IDA の文字列リストにこの値は表示されない。

---

## CFG フラット化 (`-kagura-fla`)

**Before** — 可読な if/else チェーン:

```c
int classify(int x) {
    if (x < 0)  return -1;
    if (x == 0) return 0;
    return 1;
}
```

**After** — switch ディスパッチの状態機械。静的 CFG 解析が失敗:

```c
int classify(int x) {
    uint32_t state = 0xA3F1C2B0u;   // 初期状態 (難読化済)
    int result;
    while (1) {
        switch (state) {
        case 0xA3F1C2B0u:
            state = (x < 0) ? 0x12DE4F91u : 0x7C830B22u;  break;
        case 0x12DE4F91u:
            result = -1; state = 0xFFFFFFFFu;              break;
        case 0x7C830B22u:
            state = (x == 0) ? 0x3A9E17C4u : 0x88D20F5Bu; break;
        case 0x3A9E17C4u:
            result = 0;  state = 0xFFFFFFFFu;              break;
        case 0x88D20F5Bu:
            result = 1;  state = 0xFFFFFFFFu;              break;
        case 0xFFFFFFFFu: return result;
        }
    }
}
```

---

## 算術置換 (`-kagura-sub`)

**Before:**

```llvm
%sum = add i32 %a, %b
```

**After** — 7種類の MBA 等価式からランダムに選択:

```llvm
; a + b  ≡  (a | b) + (a & b)
%or  = or  i32 %a, %b
%and = and i32 %a, %b
%sum = add i32 %or, %and
```

デコンパイラはこの式を再構築するため、元の `a + b` には戻らない。パターンマッチ式の難読化解除ツールを破ります。
