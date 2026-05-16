; RUN: %opt -load-pass-plugin=%kagura_plugin \
; RUN:   -passes="function(kagura-fla,kagura-vm),kagura-anti-debug,kagura-tamper,kagura-pac" \
; RUN:   -S %s | %FileCheck %s

; C.1: Passes that require platform-specific features or unstructured CFG must
; be no-ops on WebAssembly targets.  Verify that the function body is
; unchanged after running the full incompatible pass set.

; CHECK: define i32 @add(i32 %a, i32 %b)
; CHECK-NEXT: entry:
; CHECK-NEXT: %sum = add i32 %a, %b
; CHECK-NEXT: ret i32 %sum
; CHECK-NOT: switch
; CHECK-NOT: kagura_anti_debug_init
; CHECK-NOT: kagura_pac_key

target triple = "wasm32-unknown-unknown"

define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}
