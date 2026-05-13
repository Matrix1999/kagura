; Regression: FunctionSplit on a function where the successor of an extracted
; block has PHI nodes. The pass must fixup PHI incoming values (replace with
; UndefValue) rather than leaving dangling references to erased instructions.
; Pass: kagura-fsplit

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

define i32 @fsplit_phi_succ(i32 %x, i32 %y) {
entry:
  %cmp = icmp sgt i32 %x, 0
  br i1 %cmp, label %left, label %right
left:
  %a = mul i32 %x, 2
  br label %merge
right:
  %b = mul i32 %y, 3
  br label %merge
merge:
  %r = phi i32 [ %a, %left ], [ %b, %right ]
  ret i32 %r
}

; Larger function to exceed the size >= 5 BB threshold.
define i32 @fsplit_large_phi(i32 %n) {
entry:
  %cmp = icmp sgt i32 %n, 0
  br i1 %cmp, label %b1, label %b2
b1:
  %v1 = add i32 %n, 10
  br label %b3
b2:
  %v2 = sub i32 0, %n
  br label %b3
b3:
  %v3 = phi i32 [ %v1, %b1 ], [ %v2, %b2 ]
  %v4 = mul i32 %v3, 7
  br label %b4
b4:
  ret i32 %v4
}
