; RUN: %opt -load-pass-plugin=%kagura_plugin -passes="function(kagura-vm)" -S %s | %FileCheck %s

; VMObfuscationPass virtualises annotated functions.
; After the pass:
;   - @vm_add body must be replaced by a trampoline call to kagura_vm_execute.
;   - The original add instruction should be gone.
;   - An encrypted bytecode blob global (kagura_vm_bc_) must exist.

; CHECK: define i32 @vm_add
; CHECK: kagura_vm_execute
; CHECK: @kagura_vm_bc_
; CHECK-NOT: add nsw i32 %a, %b

; Function annotated for VM protection
@llvm.global.annotations = appending global
  [1 x { i8*, i8*, i8*, i32, i8* }]
  [{ i8*, i8*, i8*, i32, i8* }
    { i8* bitcast (i32 (i32, i32)* @vm_add to i8*),
      i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str_ann, i32 0, i32 0),
      i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.file, i32 0, i32 0),
      i32 1,
      i8* null }]

@.str_ann = private unnamed_addr constant [9 x i8] c"kagura_vm\00"
@.file = private unnamed_addr constant [7 x i8] c"test.c\00"

define i32 @vm_add(i32 %a, i32 %b) {
entry:
  %r = add nsw i32 %a, %b
  ret i32 %r
}
