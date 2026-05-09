; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-honey -kagura-honey -S %s | %FileCheck %s

; HoneyValuePass injects decoy global variables and fake stub functions.
; After the pass:
;   - At least one honey global with a recognisable pattern (g_api_secret_key
;     or similar) should appear.
;   - At least one fake stub function (validate_license / check_token / etc.)
;     should appear.
;   - A module constructor (.kagura.honey.ctor) must exist to anchor the honey
;     values and prevent dead-stripping.
;   - The original user function @main must still be present and return i32.

; CHECK: @g_api_secret_key
; CHECK: @g_auth_token
; CHECK: define {{.*}}@validate_license
; CHECK: define {{.*}}@check_token
; CHECK: define {{.*}}void @{{.*}}kagura{{.*}}honey{{.*}}ctor
; CHECK: define i32 @main

define i32 @main() {
entry:
  ret i32 0
}
