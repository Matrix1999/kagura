; Regression: passes must not crash or corrupt vararg functions.
; FunctionSplit and FLA skip vararg functions; BCF/SUB/BBR should be no-ops
; or safe on the non-vararg parts.
; Pass: kagura-bcf,kagura-sub,kagura-bbr

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

declare i32 @vsnprintf(ptr, i64, ptr, ...)

define i32 @my_printf(ptr %fmt, ...) {
entry:
  %buf = alloca [256 x i8]
  ret i32 0
}

; A normal function alongside — verify passes still run on it.
define i32 @normal_neighbour(i32 %x) {
entry:
  %y = mul i32 %x, 7
  %z = add i32 %y, 3
  ret i32 %z
}
