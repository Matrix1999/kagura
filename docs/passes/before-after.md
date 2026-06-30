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

## String Splitting (`-kagura-string-split`)

**Before** — a single string global at a contiguous offset, easy to spot:

```llvm
@api_key = private constant [29 x i8] c"this is a long secret API key"
```

A binary scan immediately reveals the secret as a contiguous span — even if
encrypted by `kagura-str` first, the *ciphertext* is still contiguous.

**After** — the literal is sliced into N random-length fragments stored in
separate globals; a flag-guarded init stub reassembles them on first use:

```llvm
@kagura_str_frag_0_0 = private constant [6 x i8] c"this i"
@kagura_str_frag_0_1 = private constant [5 x i8] c"s a l"
@kagura_str_frag_0_2 = private constant [6 x i8] c"ong se"
@kagura_str_frag_0_3 = private constant [7 x i8] c"cret AP"
@kagura_str_frag_0_4 = private constant [3 x i8] c"I k"
@kagura_str_frag_0_5 = private constant [1 x i8] c"e"
@kagura_str_frag_0_6 = private constant [1 x i8] c"y"
@kagura_str_recombined_0 = private global [29 x i8] zeroinitializer

define internal void @__kagura_strsplit_0() {
entry:
  %f = load i8, ptr @kagura_str_flag_0
  %g = icmp ne i8 %f, 0
  br i1 %g, label %done, label %init
init:
  memcpy(@kagura_str_recombined_0[0], @kagura_str_frag_0_0, 6)
  memcpy(@kagura_str_recombined_0[6], @kagura_str_frag_0_1, 5)
  ...
  store i8 1, ptr @kagura_str_flag_0
  br label %done
done:
  ret void
}
```

Compose with `kagura-str` (or `kagura-str-aes`) — the string is encrypted
first, then the ciphertext is fragmented. The binary has neither a
contiguous plaintext nor a contiguous ciphertext.

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
