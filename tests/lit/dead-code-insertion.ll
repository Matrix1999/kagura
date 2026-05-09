; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-dci -S %s | %FileCheck %s

; Dead code insertion adds unreachable blocks containing junk arithmetic.
; CHECK: define i32 @compute
; CHECK: unreachable
; CHECK: ret i32

define i32 @compute(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  br label %next

next:
  %mul = mul i32 %sum, %b
  br label %done

done:
  ret i32 %mul
}
