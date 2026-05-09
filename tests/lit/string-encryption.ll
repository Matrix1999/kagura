; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-str -S %s | %FileCheck %s

; CHECK-NOT: c"sk-test-1234567890abcdef"
; CHECK-NOT: c"https://api.example.com/v1"

; Encrypted string should be emitted as a byte array global
; CHECK: @kagura_enc_
; CHECK: internal global

@api_key = private unnamed_addr constant [25 x i8] c"sk-test-1234567890abcdef\00"
@base_url = private unnamed_addr constant [27 x i8] c"https://api.example.com/v1\00"

declare i32 @strlen(i8* nocapture) nounwind readonly

define i32 @main() {
  %len = call i32 @strlen(i8* getelementptr inbounds ([25 x i8], [25 x i8]* @api_key, i32 0, i32 0))
  ret i32 %len
}
