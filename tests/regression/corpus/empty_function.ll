; Regression: all passes must handle empty / trivial functions without crashing.
; A function with a single basic block (entry → ret) has no reorderable blocks,
; no allocas, and no strings — every pass should return PreservedAnalyses::all().
; Pass: kagura-fla,kagura-bcf,kagura-sub,kagura-bbr,kagura-mvo,kagura-pe

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

define void @empty_void() {
entry:
  ret void
}

define i32 @trivial_return() {
entry:
  ret i32 42
}

define i32 @single_add(i32 %x) {
entry:
  %r = add i32 %x, 1
  ret i32 %r
}
