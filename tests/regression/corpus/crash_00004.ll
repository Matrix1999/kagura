; Regression: LoopTransform on LLVM 17 used insertBefore(iterator) API
; which requires Instruction* argument on LLVM 17.
; Fixed: use insertBefore(&*iter) for LLVM 17 compatibility.
; Pass: kagura-lt

define i32 @loop_test(i32 %n) {
entry:
  br label %for.cond

for.cond:
  %i = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:
  %add = add nsw i32 %i, 1
  br label %for.inc

for.inc:
  %inc = add nsw i32 %i, 1
  br label %for.cond

for.end:
  ret i32 %i
}
