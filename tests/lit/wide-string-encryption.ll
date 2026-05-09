; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-wstr -kagura-wstr -S %s | %FileCheck %s

; WideStringEncryptionPass encrypts wide-character (i16/i32) string globals.
; After the pass:
;   - The plaintext content of the wide string must not be present as-is.
;   - An encrypted global (kagura_wenc_) must be emitted.
;   - A lazy-decrypt flag global (kagura_wflag_) must be emitted.
;   - The original @wide_hello global must be gone or replaced.

; CHECK-NOT: i16 72, i16 101, i16 108
; CHECK: @kagura_wenc_
; CHECK: @kagura_wflag_

; Wide string: L"Hello" as i16 array
@wide_hello = private unnamed_addr constant [6 x i16]
  [i16 72, i16 101, i16 108, i16 108, i16 111, i16 0], align 2

declare i32 @use_wstr(i16*)

define i32 @main() {
entry:
  %r = call i32 @use_wstr(i16* getelementptr inbounds ([6 x i16], [6 x i16]* @wide_hello, i32 0, i32 0))
  ret i32 %r
}
