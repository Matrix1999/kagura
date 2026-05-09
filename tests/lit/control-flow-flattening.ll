; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-fla -S %s | %FileCheck %s

; After flattening, a switch instruction must exist (state dispatcher)
; CHECK: define i32 @classify
; CHECK: switch i32
; CHECK: ret i32

define i32 @classify(i32 %x) {
entry:
  %cmp0 = icmp slt i32 %x, 0
  br i1 %cmp0, label %ret_neg, label %check_zero

check_zero:
  %cmp1 = icmp eq i32 %x, 0
  br i1 %cmp1, label %ret_zero, label %check_small

check_small:
  %cmp2 = icmp slt i32 %x, 10
  br i1 %cmp2, label %ret_one, label %ret_two

ret_neg:
  ret i32 -1

ret_zero:
  ret i32 0

ret_one:
  ret i32 1

ret_two:
  ret i32 2
}
