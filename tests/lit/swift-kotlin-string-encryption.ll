; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-wstr -kagura-wstr -S %s | %FileCheck %s

; WideStringEncryptionPass: Swift and Kotlin Native string constant coverage.
;
; After the pass:
;   - Swift string plaintext ("Hello from Swift") must be absent.
;   - Kotlin string plaintext ("Hello from Kotlin") must be absent.
;   - Swift module constructor (kagura_swift_string_decrypt_ctor) must appear.
;   - Kotlin module constructor (kagura_kotlin_string_decrypt_ctor) must appear.

; CHECK-NOT: c"Hello from Swift"
; CHECK-NOT: c"Hello from Kotlin"
; CHECK: kagura_swift_string_decrypt_ctor
; CHECK: kagura_kotlin_string_decrypt_ctor

; Swift string literal: private [N x i8] with "$s" mangled name.
@"$sSS16_builtinStringLiteralSSy_p_BpBwBitcfC.str" = private unnamed_addr constant [17 x i8] c"Hello from Swift\00"

; Kotlin Native string literal: private [N x i8] with ".kotlin" prefix.
@".kotlin_string_pool.hello" = private unnamed_addr constant [18 x i8] c"Hello from Kotlin\00"

declare i32 @use_str(ptr)

define i32 @test_swift() {
entry:
  %r = call i32 @use_str(ptr @"$sSS16_builtinStringLiteralSSy_p_BpBwBitcfC.str")
  ret i32 %r
}

define i32 @test_kotlin() {
entry:
  %r = call i32 @use_str(ptr @".kotlin_string_pool.hello")
  ret i32 %r
}
