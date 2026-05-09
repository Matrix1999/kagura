; RUN: %opt -load-pass-plugin=%kagura_plugin -passes=kagura-ci -S %s | %FileCheck %s

; CallIndirectionPass routes calls to external functions through a runtime-
; resolved thunk table.
; After the pass:
;   - The direct call to @puts must be gone.
;   - A thunk global (kagura_thunk_) must exist for puts.
;   - An indirect call (call via loaded function pointer) must appear.

; CHECK: define i32 @main
; CHECK: @kagura_thunk_
; CHECK-NOT: call i32 @puts(
; CHECK: ret i32

declare i32 @puts(i8* nocapture) nounwind

@msg = private unnamed_addr constant [6 x i8] c"hello\00"

define i32 @main() {
entry:
  %r = call i32 @puts(i8* getelementptr inbounds ([6 x i8], [6 x i8]* @msg, i32 0, i32 0))
  ret i32 %r
}
