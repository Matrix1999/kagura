# run_integration_test.cmake
# Called by CTest for each integration test.
#
# Three-stage pipeline:
#   1. clang -O2 -emit-llvm → .ll (baseline AND source for obfuscation)
#   2. opt --load-pass-plugin -passes=PASSES → obfuscated .ll
#   3. clang obfuscated.ll -o binary, then run and compare output
#
# Using opt --load-pass-plugin (not -fpass-plugin + -mllvm) because opt
# pre-scans argv for --load-pass-plugin before ParseCommandLineOptions, so
# plugin cl::opt flags are registered before the rest of argv is parsed.
#
# Parameters (passed via -D):
#   CLANG     — path to clang
#   OPT       — path to opt
#   PLUGIN    — path to libKaguraObfuscator
#   PASSES    — opt -passes= pipeline (e.g. "function(kagura-fla)")
#   SOURCE    — path to the C source file
#   EXPECTED  — expected stdout string
#   TEST_NAME — unique name for per-test temp files

cmake_minimum_required(VERSION 3.14)

# ---- Unique temp paths -------------------------------------------------------
set(BASELINE_IR  "/tmp/kagura_int_${TEST_NAME}_base.ll")
set(OBF_IR       "/tmp/kagura_int_${TEST_NAME}_obf.ll")
set(BASELINE_BIN "/tmp/kagura_int_${TEST_NAME}_base")
set(OBF_BIN      "/tmp/kagura_int_${TEST_NAME}_obf")

# ---- Compile baseline binary ------------------------------------------------
execute_process(
  COMMAND ${CLANG} -O2 ${SOURCE} -o ${BASELINE_BIN}
  RESULT_VARIABLE R ERROR_VARIABLE E
)
if(NOT R EQUAL 0)
  message(FATAL_ERROR "Baseline compile failed:\n${E}")
endif()

# ---- Run baseline -----------------------------------------------------------
execute_process(
  COMMAND ${BASELINE_BIN}
  OUTPUT_VARIABLE BASELINE_OUTPUT
  RESULT_VARIABLE R TIMEOUT 10
)
if(NOT R EQUAL 0)
  message(FATAL_ERROR "Baseline run failed (exit ${R})")
endif()

# ---- Emit IR for obfuscation ------------------------------------------------
execute_process(
  COMMAND ${CLANG} -O2 -emit-llvm -S -o ${BASELINE_IR} ${SOURCE}
  RESULT_VARIABLE R ERROR_VARIABLE E
)
if(NOT R EQUAL 0)
  message(FATAL_ERROR "IR emit failed:\n${E}")
endif()

# ---- Obfuscate with opt -----------------------------------------------------
execute_process(
  COMMAND ${OPT} --load-pass-plugin=${PLUGIN} -passes=${PASSES}
          -S -o ${OBF_IR} ${BASELINE_IR}
  RESULT_VARIABLE R ERROR_VARIABLE E
)
if(NOT R EQUAL 0)
  message(FATAL_ERROR "opt obfuscation failed (${PASSES}):\n${E}")
endif()

# ---- Compile obfuscated IR to binary ----------------------------------------
execute_process(
  COMMAND ${CLANG} ${OBF_IR} -o ${OBF_BIN}
  RESULT_VARIABLE R ERROR_VARIABLE E
)
if(NOT R EQUAL 0)
  message(FATAL_ERROR "Obfuscated IR link failed:\n${E}")
endif()

# ---- Run obfuscated binary --------------------------------------------------
execute_process(
  COMMAND ${OBF_BIN}
  OUTPUT_VARIABLE OBF_OUTPUT
  RESULT_VARIABLE R TIMEOUT 10
)
if(NOT R EQUAL 0)
  message(FATAL_ERROR "Obfuscated run failed (exit ${R})")
endif()

# ---- Compare output ---------------------------------------------------------
if(NOT BASELINE_OUTPUT STREQUAL OBF_OUTPUT)
  message(FATAL_ERROR
    "Output mismatch for pass(es) ${PASSES}!\n"
    "Expected (baseline):\n${BASELINE_OUTPUT}\n"
    "Got (obfuscated):\n${OBF_OUTPUT}\n"
  )
endif()

message(STATUS "PASS: ${PASSES} — output matches baseline")
