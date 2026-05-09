; RUN: %opt -load-pass-plugin=%kagura_plugin -passes="function(kagura-mvo)" -kagura-mvo -S %s | %FileCheck %s

; MemoryValueObfuscationPass XOR-wraps every store/load on eligible allocas.
; After obfuscation the plain constant 42 should be gone from any store and
; XOR instructions should appear for both encrypt (store side) and decrypt
; (load side).

; CHECK: define i32 @obfuscate_int
; The original constant 42 must not be stored directly.
; CHECK-NOT: store i32 42,
; An xor must appear (encrypt on store path).
; CHECK: xor i32
; CHECK: ret i32

define i32 @obfuscate_int() {
entry:
  %x = alloca i32, align 4
  store i32 42, i32* %x, align 4
  %v = load i32, i32* %x, align 4
  ret i32 %v
}

; Non-integer alloca must be left alone (no xor for the float).
; CHECK: define float @no_obfuscate_float
; CHECK-NOT: xor
; CHECK: ret float

define float @no_obfuscate_float() {
entry:
  %f = alloca float, align 4
  store float 1.0, float* %f, align 4
  %v = load float, float* %f, align 4
  ret float %v
}
