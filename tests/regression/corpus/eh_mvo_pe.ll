; Regression: MVO and PE on functions with exception handling.
; Non-EH allocas must be transformed; EH-touching allocas must be skipped.
; Pass: kagura-mvo,kagura-pe

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

declare i32 @__gxx_personality_v0(...)
declare void @might_throw()

; Integer alloca not touched by invoke — MVO should transform it.
define i32 @eh_mvo_safe(i32 %x) personality ptr @__gxx_personality_v0 {
entry:
  %slot = alloca i32
  store i32 %x, ptr %slot
  %v = load i32, ptr %slot
  invoke void @might_throw()
      to label %ok unwind label %lpad
ok:
  ret i32 %v
lpad:
  %lp = landingpad { ptr, i32 } cleanup
  ret i32 0
}

; Pointer alloca not passed to invoke — PE should transform it.
define ptr @eh_pe_safe(ptr %p) personality ptr @__gxx_personality_v0 {
entry:
  %slot = alloca ptr
  store ptr %p, ptr %slot
  %v = load ptr, ptr %slot
  invoke void @might_throw()
      to label %ok unwind label %lpad
ok:
  ret ptr %v
lpad:
  %lp = landingpad { ptr, i32 } cleanup
  ret ptr null
}
