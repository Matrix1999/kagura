# run_integration_test.cmake
# Called by CTest for each integration test.
#
# Parameters (passed via -D):
#   CLANG    — path to clang
#   PLUGIN   — path to libKaguraObfuscator
#   FLAGS    — kagura flags string (e.g. "-kagura-fla -kagura-bcf")
#   SOURCE   — path to the C source file
#   EXPECTED — expected stdout string (newlines represented as \n)

cmake_minimum_required(VERSION 3.14)

# ---- Compile baseline -------------------------------------------------------
execute_process(
  COMMAND ${CLANG} -O2 ${SOURCE} -o /tmp/kagura_int_baseline
  RESULT_VARIABLE COMPILE_RESULT
  ERROR_VARIABLE  COMPILE_ERR
)
if(NOT COMPILE_RESULT EQUAL 0)
  message(FATAL_ERROR "Baseline compile failed:\n${COMPILE_ERR}")
endif()

# ---- Run baseline -----------------------------------------------------------
execute_process(
  COMMAND /tmp/kagura_int_baseline
  OUTPUT_VARIABLE BASELINE_OUTPUT
  RESULT_VARIABLE RUN_RESULT
  TIMEOUT 10
)
if(NOT RUN_RESULT EQUAL 0)
  message(FATAL_ERROR "Baseline run failed with exit code ${RUN_RESULT}")
endif()

# ---- Compile with kagura plugin ---------------------------------------------
separate_arguments(FLAG_LIST UNIX_COMMAND "${FLAGS}")
execute_process(
  COMMAND ${CLANG} -O2 -fpass-plugin=${PLUGIN} ${FLAG_LIST}
          ${SOURCE} -o /tmp/kagura_int_obf
  RESULT_VARIABLE OBF_COMPILE_RESULT
  ERROR_VARIABLE  OBF_COMPILE_ERR
)
if(NOT OBF_COMPILE_RESULT EQUAL 0)
  message(FATAL_ERROR "Obfuscated compile failed (${FLAGS}):\n${OBF_COMPILE_ERR}")
endif()

# ---- Run obfuscated binary --------------------------------------------------
execute_process(
  COMMAND /tmp/kagura_int_obf
  OUTPUT_VARIABLE OBF_OUTPUT
  RESULT_VARIABLE OBF_RUN_RESULT
  TIMEOUT 10
)
if(NOT OBF_RUN_RESULT EQUAL 0)
  message(FATAL_ERROR "Obfuscated run failed with exit code ${OBF_RUN_RESULT}")
endif()

# ---- Compare output ---------------------------------------------------------
if(NOT BASELINE_OUTPUT STREQUAL OBF_OUTPUT)
  message(FATAL_ERROR
    "Output mismatch for pass(es) ${FLAGS}!\n"
    "Expected (baseline):\n${BASELINE_OUTPUT}\n"
    "Got (obfuscated):\n${OBF_OUTPUT}\n"
  )
endif()

message(STATUS "PASS: ${FLAGS} — output matches baseline")
