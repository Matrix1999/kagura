; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-genc -S %s | %FileCheck %s

; GlobalEncryptionPass XOR-encrypts integer globals and patches load sites.
; After the pass:
;   - The plaintext value 0xDEADBEEF must not appear as a direct global initializer.
;   - An xor instruction must be present to decrypt on load.

; CHECK: define i32 @get_secret
; CHECK: xor i32
; CHECK: ret i32

@g_secret = internal global i32 -559038737  ; 0xDEADBEEF

define i32 @get_secret() {
entry:
  %v = load i32, i32* @g_secret, align 4
  ret i32 %v
}
