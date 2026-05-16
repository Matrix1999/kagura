; Regression: VMObfuscation on a function whose bytecode would exceed the old
; 64 KB (uint16) jump offset limit. With uint32 offsets this must succeed.
; Pass: kagura-vm

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

; 80-block chain — each block emits several bytecode instructions,
; comfortably exceeding 64 KB of VM bytecode with the old encoding.
define i64 @vm_large(i64 %x) {
entry:
  br label %b0
b0:  %v0  = add i64 %x,   0  br label %b1
b1:  %v1  = add i64 %v0,  1  br label %b2
b2:  %v2  = add i64 %v1,  2  br label %b3
b3:  %v3  = add i64 %v2,  3  br label %b4
b4:  %v4  = add i64 %v3,  4  br label %b5
b5:  %v5  = add i64 %v4,  5  br label %b6
b6:  %v6  = add i64 %v5,  6  br label %b7
b7:  %v7  = add i64 %v6,  7  br label %b8
b8:  %v8  = add i64 %v7,  8  br label %b9
b9:  %v9  = add i64 %v8,  9  br label %b10
b10: %v10 = add i64 %v9,  10 br label %b11
b11: %v11 = add i64 %v10, 11 br label %b12
b12: %v12 = add i64 %v11, 12 br label %b13
b13: %v13 = add i64 %v12, 13 br label %b14
b14: %v14 = add i64 %v13, 14 br label %b15
b15: %v15 = add i64 %v14, 15 br label %b16
b16: %v16 = add i64 %v15, 16 br label %b17
b17: %v17 = add i64 %v16, 17 br label %b18
b18: %v18 = add i64 %v17, 18 br label %b19
b19: %v19 = add i64 %v18, 19 br label %b20
b20: %v20 = add i64 %v19, 20 br label %b21
b21: %v21 = add i64 %v20, 21 br label %b22
b22: %v22 = add i64 %v21, 22 br label %b23
b23: %v23 = add i64 %v22, 23 br label %b24
b24: %v24 = add i64 %v23, 24 br label %b25
b25: %v25 = add i64 %v24, 25 br label %b26
b26: %v26 = add i64 %v25, 26 br label %b27
b27: %v27 = add i64 %v26, 27 br label %b28
b28: %v28 = add i64 %v27, 28 br label %b29
b29: %v29 = add i64 %v28, 29 br label %b30
b30: %v30 = add i64 %v29, 30 br label %b31
b31: %v31 = add i64 %v30, 31 br label %b32
b32: %v32 = add i64 %v31, 32 br label %b33
b33: %v33 = add i64 %v32, 33 br label %b34
b34: %v34 = add i64 %v33, 34 br label %b35
b35: %v35 = add i64 %v34, 35 br label %b36
b36: %v36 = add i64 %v35, 36 br label %b37
b37: %v37 = add i64 %v36, 37 br label %b38
b38: %v38 = add i64 %v37, 38 br label %b39
b39: %v39 = add i64 %v38, 39 ret i64 %v39
}
