find_package(LLVM REQUIRED CONFIG)

# Minimal test: compile a C file with the plugin and verify it doesn't crash
add_custom_target(check-kagura
  COMMAND ${CMAKE_COMMAND} -E echo "Running kagura integration tests..."
  DEPENDS KaguraObfuscator
)

# Test helper: compile a simple C source with all passes enabled
set(KAGURA_PLUGIN $<TARGET_FILE:KaguraObfuscator>)
set(LLVM_BIN ${LLVM_TOOLS_BINARY_DIR})

# Use opt --load-pass-plugin -passes= for testing, which works across all
# LLVM versions (avoids -mllvm cl::opt registration timing issues in LLVM 19).
# Each test: clang -O0 -emit-llvm → opt --load-pass-plugin -passes=...

# Helper macro: create a two-stage test (C → IR → opt with pass)
macro(kagura_add_pass_test TEST_NAME PASS_PIPELINE SOURCE_FILE)
  add_test(NAME ${TEST_NAME}
    COMMAND ${CMAKE_COMMAND}
      -DCLANG=${LLVM_BIN}/clang
      -DOPT=${LLVM_BIN}/opt
      -DPLUGIN=${KAGURA_PLUGIN}
      -DPASSES=${PASS_PIPELINE}
      -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/inputs/${SOURCE_FILE}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/run_pass_test.cmake
  )
endmacro()

kagura_add_pass_test(test_string_encryption "kagura-str" string_test.c)
kagura_add_pass_test(test_flattening "function(kagura-fla)" control_flow_test.c)
kagura_add_pass_test(test_bogus_cf "function(kagura-bcf)" control_flow_test.c)
kagura_add_pass_test(test_substitution "function(kagura-sub)" arithmetic_test.c)
kagura_add_pass_test(test_vm_vm "function(kagura-vm)" vm_test.c)
kagura_add_pass_test(test_indirect_branch "function(kagura-ibr)" indirect_branch_test.c)
kagura_add_pass_test(test_loop_transform "function(kagura-lt)" loop_test.c)
kagura_add_pass_test(test_pointer "function(kagura-bcf,kagura-sub,kagura-ibr,kagura-lt)" pointer_test.c)
kagura_add_pass_test(test_basic_block_reordering "function(kagura-bbr)" control_flow_test.c)
kagura_add_pass_test(test_basic_block_splitting "function(kagura-bbs)" arithmetic_test.c)
kagura_add_pass_test(test_dead_code_insertion "function(kagura-dci)" control_flow_test.c)
kagura_add_pass_test(test_symbol_visibility "kagura-sv" arithmetic_test.c)
kagura_add_pass_test(test_all_passes
  "kagura-str,kagura-fsplit,kagura-sv,function(kagura-bcf,kagura-sub,kagura-co,kagura-ibr,kagura-lt,kagura-bbr,kagura-bbs,kagura-dci)"
  combined_test.c)

# ── New pass tests (Sprint 5 / 4.7.1) ────────────────────────────────────────
kagura_add_pass_test(test_mvo
  "function(kagura-mvo)"
  arithmetic_test.c)
kagura_add_pass_test(test_honey
  "kagura-honey"
  arithmetic_test.c)
kagura_add_pass_test(test_wstr
  "kagura-wstr"
  string_test.c)
kagura_add_pass_test(test_str_aes
  "kagura-str-aes"
  string_test.c)
kagura_add_pass_test(test_global_enc
  "kagura-genc"
  arithmetic_test.c)
kagura_add_pass_test(test_call_indirection
  "kagura-ci"
  combined_test.c)
kagura_add_pass_test(test_function_split
  "kagura-fsplit"
  combined_test.c)

# ── FileCheck / lit tests ──────────────────────────────────────────────────────

find_program(LIT_EXECUTABLE NAMES lit llvm-lit
  HINTS ${LLVM_TOOLS_BINARY_DIR} ${LLVM_BIN}
  DOC "LLVM lit test runner"
)

find_program(FILECHECK_EXECUTABLE NAMES FileCheck
  HINTS ${LLVM_TOOLS_BINARY_DIR}
  DOC "LLVM FileCheck"
)

# ---- 4.7.2 Integration tests ------------------------------------------------
add_subdirectory(integration)

# ---- 4.7.5 Fuzz targets (only when KAGURA_BUILD_FUZZ=ON) -------------------
add_subdirectory(fuzz)

# ---- 4.8.7 Regression corpus -----------------------------------------------
add_subdirectory(regression)

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
