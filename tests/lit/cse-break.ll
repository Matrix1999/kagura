; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-cse-break -S %s | %FileCheck %s

; The CSE-break pass duplicates shared SSA expressions so each user sees a
; fresh definition. After the pass we expect to see MORE `add` instructions
; than we started with — the shared `%t = add ...` is duplicated.
;
; Strategy: count occurrences of the marker. The original IR has exactly one
; add; the obfuscated IR should have at least two.

; CHECK: define i32 @arith
; CHECK: add i32 %a, %b
; CHECK: add i32 %a, %b
; CHECK: ret i32

define i32 @arith(i32 %a, i32 %b) {
entry:
  %t = add i32 %a, %b
  %x = mul i32 %t, 2
  %y = sub i32 %t, 3
  %z = add i32 %x, %y
  ret i32 %z
}

; Negative test: a single-use binop should not be duplicated (nothing to break).
; The original `%u = and i32 %a, %b` has exactly one use, so after the pass
; there's still only one `and`.

; CHECK: define i32 @single_use
; CHECK: and i32 %a, %b
; CHECK-NOT: and i32 %a, %b
; CHECK: ret i32

define i32 @single_use(i32 %a, i32 %b) {
entry:
  %u = and i32 %a, %b
  ret i32 %u
}
