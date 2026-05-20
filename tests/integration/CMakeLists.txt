# Integration tests — compile a realistic "app" through kagura passes
# and verify semantic correctness (output matches baseline) and binary size.
#
# Each integration test:
#   1. Compiles the test subject WITHOUT kagura → baseline binary
#   2. Compiles the same source to IR, obfuscates with opt --load-pass-plugin,
#      then links the obfuscated bitcode into an obfuscated binary
#   3. Runs both; compares stdout output
#
# We use opt --load-pass-plugin (rather than -fpass-plugin + -mllvm) because
# opt pre-scans argv for --load-pass-plugin before calling
# ParseCommandLineOptions, so the plugin's cl::opt options are already
# registered when the rest of argv is parsed.  With -fpass-plugin the plugin
# is loaded late (during pipeline construction), after LLVM option parsing, so
# any -mllvm flags referencing kagura cl::opts arrive too early and are
# rejected as unknown on all LLVM versions.

cmake_minimum_required(VERSION 3.20)

set(KAGURA_PLUGIN $<TARGET_FILE:KaguraObfuscator>)
set(LLVM_BIN ${LLVM_TOOLS_BINARY_DIR})
set(INT_INPUTS ${CMAKE_CURRENT_SOURCE_DIR}/subjects)

# PASSES is an opt -passes= pipeline string, e.g. "function(kagura-fla)" or
# "kagura-str,function(kagura-fla,kagura-co)".
macro(kagura_add_integration_test TEST_NAME PASSES SOURCE_FILE EXPECTED_OUTPUT)
  add_test(NAME integration_${TEST_NAME}
    COMMAND ${CMAKE_COMMAND}
      -DCLANG=${LLVM_BIN}/clang
      -DOPT=${LLVM_BIN}/opt
      -DPLUGIN=${KAGURA_PLUGIN}
      -DPASSES=${PASSES}
      -DSOURCE=${INT_INPUTS}/${SOURCE_FILE}
      -DEXPECTED=${EXPECTED_OUTPUT}
      -DTEST_NAME=${TEST_NAME}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/run_integration_test.cmake
  )
  set_tests_properties(integration_${TEST_NAME} PROPERTIES
    DEPENDS KaguraObfuscator
    TIMEOUT 60
  )
endmacro()

# ---- Core CFG obfuscation ---------------------------------------------------
kagura_add_integration_test(fla_correctness
  "function(kagura-fla)"
  fibonacci.c
  "55\n89\n144\n")

kagura_add_integration_test(bcf_correctness
  "function(kagura-bcf)"
  fibonacci.c
  "55\n89\n144\n")

kagura_add_integration_test(sub_correctness
  "function(kagura-sub)"
  arithmetic_smoke.c
  "42\n100\n0\n-1\n")

kagura_add_integration_test(fla_bcf_sub_combined
  "function(kagura-fla,kagura-bcf,kagura-sub)"
  fibonacci.c
  "55\n89\n144\n")

# ---- Data obfuscation -------------------------------------------------------
kagura_add_integration_test(str_encrypt_correctness
  "kagura-str"
  string_smoke.c
  "hello world\nkagura obfuscator\n")

kagura_add_integration_test(co_correctness
  "function(kagura-co)"
  arithmetic_smoke.c
  "42\n100\n0\n-1\n")

# ---- CFG fragmentation ------------------------------------------------------
kagura_add_integration_test(bbr_correctness
  "function(kagura-bbr)"
  fibonacci.c
  "55\n89\n144\n")

kagura_add_integration_test(bbs_correctness
  "function(kagura-bbs)"
  arithmetic_smoke.c
  "42\n100\n0\n-1\n")

kagura_add_integration_test(dci_correctness
  "function(kagura-dci)"
  fibonacci.c
  "55\n89\n144\n")

# ---- All-pass combination ---------------------------------------------------
kagura_add_integration_test(all_passes_correctness
  "kagura-str,function(kagura-fla,kagura-bcf,kagura-sub,kagura-co,kagura-bbr,kagura-bbs,kagura-dci)"
  fibonacci.c
  "55\n89\n144\n")

# ---- Loop transform ---------------------------------------------------------
kagura_add_integration_test(lt_correctness
  "function(kagura-lt)"
  loop_smoke.c
  "45\n550\n")
