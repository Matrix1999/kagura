; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-sub -S %s | %FileCheck %s

; The substitution pass replaces add/sub/and/or/xor with MBA equivalents.
; After substitution, a plain `add` on these operands should be gone.
; CHECK: define i32 @arith
; CHECK-NOT: add nsw i32 %a, %b
; CHECK: ret i32

define i32 @arith(i32 %a, i32 %b) {
entry:
  %add = add nsw i32 %a, %b
  %sub = sub nsw i32 %add, %b
  %and = and i32 %sub, %a
  %or  = or  i32 %and, %b
  %xor = xor i32 %or, %a
  ret i32 %xor
}
