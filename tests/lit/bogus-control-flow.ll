; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-bcf -S %s | %FileCheck %s

; After bogus CF, the function must still have a ret instruction (correctness)
; CHECK: define i32 @classify
; CHECK: ret i32

; Bogus blocks use a conditional branch on an opaque predicate
; CHECK: br i1

define i32 @classify(i32 %x) {
entry:
  %cmp_neg = icmp slt i32 %x, 0
  br i1 %cmp_neg, label %ret_neg, label %check_zero

check_zero:
  %cmp_zero = icmp eq i32 %x, 0
  br i1 %cmp_zero, label %ret_zero, label %ret_pos

ret_neg:
  ret i32 -1

ret_zero:
  ret i32 0

ret_pos:
  ret i32 1
}
