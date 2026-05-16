; Regression: BBR on a function with EH — landing-pad and unwind-dest blocks
; must not be moved; non-EH blocks must be reorderable without corrupting IR.
; Pass: kagura-bbr

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

declare i32 @__gxx_personality_v0(...)
declare void @might_throw()
declare void @cleanup_fn()

define i32 @eh_bbr_basic(i32 %x) personality ptr @__gxx_personality_v0 {
entry:
  %cmp = icmp sgt i32 %x, 0
  br i1 %cmp, label %pos, label %neg
pos:
  %a = mul i32 %x, 3
  invoke void @might_throw()
      to label %cont unwind label %lpad
neg:
  %b = mul i32 %x, -1
  br label %done
cont:
  %c = add i32 %a, 1
  br label %done
lpad:
  %lp = landingpad { ptr, i32 } cleanup
  call void @cleanup_fn()
  br label %done
done:
  %r = phi i32 [ %c, %cont ], [ %b, %neg ], [ 0, %lpad ]
  ret i32 %r
}
