; Deterministic output guarantee (B.3): given the same IR and seed, kagura
; must produce byte-identical output so obfuscated builds are reproducible.
;
; 1. With no explicit seed, the default seed is fixed, so two independent runs
;    over the same module are byte-identical.
; 2. With an explicit -kagura-seed, runs are likewise reproducible.
; Randomness-consuming passes are exercised: string/global encryption (keys)
; and instruction substitution (random MBA pattern selection).

; RUN: %opt -load-pass-plugin=%kagura_plugin -passes="kagura-str,kagura-genc,function(kagura-sub)" -S %s -o %t.default.1.ll
; RUN: %opt -load-pass-plugin=%kagura_plugin -passes="kagura-str,kagura-genc,function(kagura-sub)" -S %s -o %t.default.2.ll
; RUN: diff %t.default.1.ll %t.default.2.ll

; RUN: %opt -load-pass-plugin=%kagura_plugin -kagura-seed=1234 -passes="kagura-str,function(kagura-sub)" -S %s -o %t.seed.1.ll
; RUN: %opt -load-pass-plugin=%kagura_plugin -kagura-seed=1234 -passes="kagura-str,function(kagura-sub)" -S %s -o %t.seed.2.ll
; RUN: diff %t.seed.1.ll %t.seed.2.ll

@api_key  = private unnamed_addr constant [25 x i8] c"sk-test-1234567890abcdef\00"
@base_url = private unnamed_addr constant [27 x i8] c"https://api.example.com/v1\00"
@counter  = internal global i32 0

declare i32 @puts(ptr)

define i32 @compute(i32 %a, i32 %b) {
entry:
  %s = add i32 %a, %b
  %x = xor i32 %s, 305419896
  %m = mul i32 %x, %b
  %r = sub i32 %m, %a
  ret i32 %r
}

define i32 @main() {
  call i32 @puts(ptr @api_key)
  call i32 @puts(ptr @base_url)
  %v = load i32, ptr @counter
  %c = call i32 @compute(i32 %v, i32 7)
  ret i32 %c
}
