; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-str-aes -kagura-str-aes -S %s | %FileCheck %s

; StringEncryptionAESPass encrypts string literals with AES-128-CTR.
; After the pass:
;   - The plaintext string content must not appear.
;   - An AES-encrypted blob global (kagura_aes_enc_) must be emitted.
;   - A runtime decryption call (kagura_aes_decrypt_str) must be injected.

; CHECK-NOT: c"supersecret\00"
; CHECK: @kagura_aes_enc_
; CHECK: kagura_aes_decrypt_str

@secret = private unnamed_addr constant [12 x i8] c"supersecret\00"

declare i32 @strlen(i8* nocapture) nounwind readonly

define i32 @main() {
entry:
  %len = call i32 @strlen(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @secret, i32 0, i32 0))
  ret i32 %len
}
