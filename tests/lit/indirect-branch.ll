; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-ibr -S %s | %FileCheck %s

; IndirectBranch replaces direct calls to internal functions with indirect calls
; through a function-pointer global.
; CHECK: define i32 @caller
; CHECK: load {{.*}}ptr
; CHECK: call i32 %
; CHECK-NOT: call i32 @callee(

define internal i32 @callee(i32 %x) {
  %r = mul i32 %x, 2
  ret i32 %r
}

define i32 @caller(i32 %n) {
  %r = call i32 @callee(i32 %n)
  ret i32 %r
}
