# Before / After Examples

What an attacker actually sees, before and after Kagura.

---

## String Encryption (`-kagura-str`)

**Before** — string literal in plaintext `.rodata`:

```llvm
@api_key = private constant [33 x i8] c"sk-prod-9f2a1c3e8b4d7f0e1a2c3d4e5f6a7b8c\00"

define void @connect() {
  call void @send_auth(ptr @api_key)
}
```

**After** — XOR-encrypted blob; decrypted on first call, zeroed immediately after:

```llvm
@api_key.enc = private constant [33 x i8] c"\xde\xad\x7f\x12..."  ; encrypted
@api_key.dec = global [33 x i8] zeroinitializer                    ; plaintext lives here only briefly

define void @connect() {
  ; injected decrypt stub — checks flag, XORs blob into .dec, calls send_auth, zeros .dec
  call void @__kagura_str_init_0()
  call void @send_auth(ptr @api_key.dec)
}
```

`strings` on the binary returns garbage. IDA's string list is empty for this
value.

---

## CFG Flattening (`-kagura-fla`)

**Before** — readable if/else chain:

```c
int classify(int x) {
    if (x < 0)  return -1;
    if (x == 0) return 0;
    return 1;
}
```

**After** — switch-dispatched state machine; static CFG analysis fails:

```c
int classify(int x) {
    uint32_t state = 0xA3F1C2B0u;   // initial state (obfuscated)
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

## CSE Break (`-kagura-cse-break`)

**Before** — clang `-O2` shares a common subexpression across users:

```llvm
%t = add i32 %a, %b
%x = mul i32 %t, 2
%y = sub i32 %t, 3
```

Decompilers easily re-fold this back to readable `t = a + b; x = t*2; y = t - 3`.

**After** — each user gets its own private copy:

```llvm
%t = add i32 %a, %b           ; original — first use keeps it
%x = mul i32 %t, 2
%cse.break = add i32 %a, %b   ; fresh clone for the second use
%y = sub i32 %cse.break, 3
```

Functionally identical, but Ghidra / IDA hex-rays / Binary Ninja MLIL all
report this as **two separate computations** during decompilation, hurting
readability of the recovered C.

---

## Arithmetic Substitution (`-kagura-sub`)

**Before:**

```llvm
%sum = add i32 %a, %b
```

**After** — one of 7 MBA equivalents selected at random:

```llvm
; a + b  ≡  (a | b) + (a & b)
%or  = or  i32 %a, %b
%and = and i32 %a, %b
%sum = add i32 %or, %and
```

Decompilers reconstruct the expression, not the original `a + b`, breaking
pattern-matching deobfuscators.
