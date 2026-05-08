find_package(LLVM REQUIRED CONFIG)

# Minimal test: compile a C file with the plugin and verify it doesn't crash
add_custom_target(check-kagura
  COMMAND ${CMAKE_COMMAND} -E echo "Running kagura integration tests..."
  DEPENDS KaguraObfuscator
)

# Test helper: compile a simple C source with all passes enabled
set(KAGURA_PLUGIN $<TARGET_FILE:KaguraObfuscator>)
set(LLVM_BIN ${LLVM_TOOLS_BINARY_DIR})

add_test(NAME test_string_encryption
  COMMAND ${LLVM_BIN}/clang
    -fpass-plugin=${KAGURA_PLUGIN}
    -mllvm -kagura-str
    -O1
    -o /dev/null
    ${CMAKE_CURRENT_SOURCE_DIR}/inputs/string_test.c
)

add_test(NAME test_flattening
  COMMAND ${LLVM_BIN}/clang
    -fpass-plugin=${KAGURA_PLUGIN}
    -mllvm -kagura-fla
    -O1
    -o /dev/null
    ${CMAKE_CURRENT_SOURCE_DIR}/inputs/control_flow_test.c
)

add_test(NAME test_bogus_cf
  COMMAND ${LLVM_BIN}/clang
    -fpass-plugin=${KAGURA_PLUGIN}
    -mllvm -kagura-bcf
    -mllvm -kagura-bcf-prob=50
    -O1
    -o /dev/null
    ${CMAKE_CURRENT_SOURCE_DIR}/inputs/control_flow_test.c
)

add_test(NAME test_substitution
  COMMAND ${LLVM_BIN}/clang
    -fpass-plugin=${KAGURA_PLUGIN}
    -mllvm -kagura-sub
    -O1
    -o /dev/null
    ${CMAKE_CURRENT_SOURCE_DIR}/inputs/arithmetic_test.c
)

# Runtime library (vm_interpreter, anti_debug)
set(RUNTIME_LIB $<TARGET_FILE:kagura_runtime>)

add_test(NAME test_vm
  COMMAND ${LLVM_BIN}/clang
    -fpass-plugin=${KAGURA_PLUGIN}
    -mllvm -kagura-vm
    -O1
    -o /dev/null
    ${CMAKE_CURRENT_SOURCE_DIR}/inputs/vm_test.c
    ${RUNTIME_LIB}
)

add_test(NAME test_indirect_branch
  COMMAND ${LLVM_BIN}/clang
    -fpass-plugin=${KAGURA_PLUGIN}
    -mllvm -kagura-ibr
    -O1
    -o /dev/null
    ${CMAKE_CURRENT_SOURCE_DIR}/inputs/indirect_branch_test.c
)

add_test(NAME test_loop_transform
  COMMAND ${LLVM_BIN}/clang
    -fpass-plugin=${KAGURA_PLUGIN}
    -mllvm -kagura-lt
    -O1
    -o /dev/null
    ${CMAKE_CURRENT_SOURCE_DIR}/inputs/loop_test.c
)

add_test(NAME test_pointer
  COMMAND ${LLVM_BIN}/clang
    -fpass-plugin=${KAGURA_PLUGIN}
    -mllvm -kagura-fla
    -mllvm -kagura-bcf
    -mllvm -kagura-sub
    -mllvm -kagura-ibr
    -mllvm -kagura-lt
    -O1
    -o /dev/null
    ${CMAKE_CURRENT_SOURCE_DIR}/inputs/pointer_test.c
)

add_test(NAME test_all_passes
  COMMAND ${LLVM_BIN}/clang
    -fpass-plugin=${KAGURA_PLUGIN}
    -mllvm -kagura-fla -mllvm -kagura-bcf -mllvm -kagura-sub
    -mllvm -kagura-str -mllvm -kagura-co -mllvm -kagura-ibr
    -mllvm -kagura-lt -mllvm -kagura-fsplit
    -O1
    -o /dev/null
    ${CMAKE_CURRENT_SOURCE_DIR}/inputs/combined_test.c
)

# ── FileCheck / lit tests ──────────────────────────────────────────────────────

find_program(LIT_EXECUTABLE NAMES lit llvm-lit
  HINTS ${LLVM_TOOLS_BINARY_DIR} ${LLVM_BIN}
  DOC "LLVM lit test runner"
)

find_program(FILECHECK_EXECUTABLE NAMES FileCheck
  HINTS ${LLVM_TOOLS_BINARY_DIR}
  DOC "LLVM FileCheck"
)

if(LIT_EXECUTABLE AND FILECHECK_EXECUTABLE)
  set(KAGURA_PLUGIN_PATH $<TARGET_FILE:KaguraObfuscator>)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/lit/lit.site.cfg.py
    @ONLY
  )

  add_test(NAME lit-filecheck
    COMMAND ${LIT_EXECUTABLE} -v ${CMAKE_CURRENT_SOURCE_DIR}/lit/
      --param kagura_plugin=${KAGURA_PLUGIN_PATH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_tests_properties(lit-filecheck PROPERTIES
    DEPENDS KaguraObfuscator
  )
else()
  message(STATUS "lit or FileCheck not found — skipping FileCheck tests")
endif()
