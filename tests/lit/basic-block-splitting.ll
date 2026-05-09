; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-bbs -S %s | %FileCheck %s

; BasicBlockSplitting splits large blocks with unconditional branches.
; After splitting the function must still have a valid ret and at least one
; additional unconditional branch (split point).

; CHECK: define i32 @heavy_block
; CHECK: br label
; CHECK: ret i32

define i32 @heavy_block(i32 %a, i32 %b, i32 %c) {
entry:
  %t0 = add i32 %a, %b
  %t1 = sub i32 %t0, %c
  %t2 = mul i32 %t1, %a
  %t3 = xor i32 %t2, %b
  %t4 = add i32 %t3, %c
  %t5 = sub i32 %t4, %a
  %t6 = mul i32 %t5, %b
  ret i32 %t6
}
