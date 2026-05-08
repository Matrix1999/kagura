; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-sv -S %s | %FileCheck %s

; SymbolVisibilityPass sets internal/private functions to hidden visibility.
; The internal function @helper should become hidden_visibility.
; The external function @external_api must NOT be changed.

; CHECK: define hidden {{.*}}@helper
; CHECK: define {{(dso_local )?}}i32 @external_api

define internal i32 @helper(i32 %x) {
  %r = mul i32 %x, 3
  ret i32 %r
}

define i32 @external_api(i32 %n) {
  %r = call i32 @helper(i32 %n)
  ret i32 %r
}
