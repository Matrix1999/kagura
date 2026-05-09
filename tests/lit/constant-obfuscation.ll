; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-co -S %s | %FileCheck %s

; Constant obfuscation replaces integer constants with MBA expressions.
; A plain `ret i32 42` should no longer appear.
; CHECK: define i32 @get_magic
; CHECK-NOT: ret i32 42
; CHECK: ret i32

define i32 @get_magic() {
  ret i32 42
}

define i32 @use_constants(i32 %x) {
  ; CHECK-NOT: add i32 %x, 100
  %a = add i32 %x, 100
  ; CHECK-NOT: mul i32 %a, 7
  %b = mul i32 %a, 7
  ret i32 %b
}
