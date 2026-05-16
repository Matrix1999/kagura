; Regression: FLA on a function with many PHI nodes.
; reg2mem (demotePhis) must eliminate all PHIs before flattening;
; any leftover PHI after demote causes an SSA dominance violation.
; Pass: kagura-fla

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

define i32 @phi_chain(i32 %x, i32 %y, i32 %z) {
entry:
  %cmp1 = icmp sgt i32 %x, 0
  br i1 %cmp1, label %a, label %b
a:
  %va = add i32 %x, %y
  br label %c
b:
  %vb = sub i32 %y, %z
  br label %c
c:
  %vc = phi i32 [ %va, %a ], [ %vb, %b ]
  %cmp2 = icmp sgt i32 %vc, 10
  br i1 %cmp2, label %d, label %e
d:
  %vd = mul i32 %vc, 2
  br label %f
e:
  %ve = add i32 %vc, 3
  br label %f
f:
  %vf = phi i32 [ %vd, %d ], [ %ve, %e ]
  ret i32 %vf
}
