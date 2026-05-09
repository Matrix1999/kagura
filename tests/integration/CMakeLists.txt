# 4.7.2: Integration tests — compile a realistic "app" through kagura passes
# and verify semantic correctness (output matches baseline) and binary size.
#
# Each integration test:
#   1. Compiles the test subject WITHOUT kagura → baseline binary
#   2. Compiles WITH a selected pass combination → obfuscated binary
#   3. Runs both; compares stdout output
#   4. Optionally checks binary size does not regress beyond a threshold

cmake_minimum_required(VERSION 3.20)

set(KAGURA_PLUGIN $<TARGET_FILE:KaguraObfuscator>)
set(LLVM_BIN ${LLVM_TOOLS_BINARY_DIR})
set(INT_INPUTS ${CMAKE_CURRENT_SOURCE_DIR}/subjects)

macro(kagura_add_integration_test TEST_NAME FLAGS_LIST SOURCE_FILE EXPECTED_OUTPUT)
  add_test(NAME integration_${TEST_NAME}
    COMMAND ${CMAKE_COMMAND}
      -DCLANG=${LLVM_BIN}/clang
      -DPLUGIN=${KAGURA_PLUGIN}
      -DFLAGS=${FLAGS_LIST}
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
  "-kagura-fla"
  fibonacci.c
  "55\n89\n144\n")

kagura_add_integration_test(bcf_correctness
  "-kagura-bcf"
  fibonacci.c
  "55\n89\n144\n")

kagura_add_integration_test(sub_correctness
  "-kagura-sub"
  arithmetic_smoke.c
  "42\n100\n0\n-1\n")

kagura_add_integration_test(fla_bcf_sub_combined
  "-kagura-fla -kagura-bcf -kagura-sub"
  fibonacci.c
  "55\n89\n144\n")

# ---- Data obfuscation -------------------------------------------------------
kagura_add_integration_test(str_encrypt_correctness
  "-kagura-str"
  string_smoke.c
  "hello world\nkagura obfuscator\n")

kagura_add_integration_test(co_correctness
  "-kagura-co"
  arithmetic_smoke.c
  "42\n100\n0\n-1\n")

# ---- CFG fragmentation ------------------------------------------------------
kagura_add_integration_test(bbr_correctness
  "-kagura-bbr"
  fibonacci.c
  "55\n89\n144\n")

kagura_add_integration_test(bbs_correctness
  "-kagura-bbs"
  arithmetic_smoke.c
  "42\n100\n0\n-1\n")

kagura_add_integration_test(dci_correctness
  "-kagura-dci"
  fibonacci.c
  "55\n89\n144\n")

# ---- All-pass combination ---------------------------------------------------
kagura_add_integration_test(all_passes_correctness
  "-kagura-fla -kagura-bcf -kagura-sub -kagura-str -kagura-co -kagura-bbr -kagura-bbs -kagura-dci"
  fibonacci.c
  "55\n89\n144\n")

# ---- Loop transform ---------------------------------------------------------
kagura_add_integration_test(lt_correctness
  "-kagura-lt"
  loop_smoke.c
  "45\n550\n")
