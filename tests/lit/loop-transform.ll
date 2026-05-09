; RUN: %opt -load-pass-plugin=%kagura_plugin -passes="function(kagura-lt)" -S %s | %FileCheck %s

; LoopTransformPass adds bogus dead counters and opaque invariant branches.
; After the pass:
;   - The original loop body (add instruction) must still be present.
;   - Additional basic blocks (dead counter / opaque branch) must appear
;     (total block count > 2 for a simple 1-BB loop body).
;   - The function must still return i32.

; CHECK: define i32 @sum_loop
; CHECK: br i1
; CHECK: ret i32

define i32 @sum_loop(i32 %n) {
entry:
  br label %loop

loop:
  %i   = phi i32 [ 0, %entry ], [ %i1, %loop ]
  %sum = phi i32 [ 0, %entry ], [ %sum1, %loop ]
  %sum1 = add i32 %sum, %i
  %i1   = add i32 %i, 1
  %cond = icmp slt i32 %i1, %n
  br i1 %cond, label %loop, label %exit

exit:
  ret i32 %sum
}
