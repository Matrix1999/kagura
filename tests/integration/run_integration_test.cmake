# run_integration_test.cmake
# Called by CTest for each integration test.
#
# Parameters (passed via -D):
#   CLANG     — path to clang
#   PLUGIN    — path to libKaguraObfuscator
#   FLAGS     — kagura flags string (e.g. "-kagura-fla -kagura-bcf")
#   SOURCE    — path to the C source file
#   EXPECTED  — expected stdout string (newlines represented as \n)
#   TEST_NAME — unique test name used to create per-test temp files

cmake_minimum_required(VERSION 3.14)

# ---- Unique temp paths (avoid collisions when tests run in parallel) ---------
set(BASELINE_BIN "/tmp/kagura_int_${TEST_NAME}_base")
set(OBF_BIN      "/tmp/kagura_int_${TEST_NAME}_obf")

# ---- Compile baseline -------------------------------------------------------
execute_process(
  COMMAND ${CLANG} -O2 ${SOURCE} -o ${BASELINE_BIN}
  RESULT_VARIABLE COMPILE_RESULT
  ERROR_VARIABLE  COMPILE_ERR
)
if(NOT COMPILE_RESULT EQUAL 0)
  message(FATAL_ERROR "Baseline compile failed:\n${COMPILE_ERR}")
endif()

# ---- Run baseline -----------------------------------------------------------
execute_process(
  COMMAND ${BASELINE_BIN}
  OUTPUT_VARIABLE BASELINE_OUTPUT
  RESULT_VARIABLE RUN_RESULT
  TIMEOUT 10
)
if(NOT RUN_RESULT EQUAL 0)
  message(FATAL_ERROR "Baseline run failed with exit code ${RUN_RESULT}")
endif()

# ---- Compile with kagura plugin ---------------------------------------------
# kagura flags are cl::opt options registered inside the plugin.  When using
# -fpass-plugin they must be forwarded via -mllvm so LLVM's internal command-
# line parser sees them (not clang's frontend, which rejects unknown flags).
separate_arguments(FLAG_LIST UNIX_COMMAND "${FLAGS}")
set(MLLVM_FLAGS)
foreach(F IN LISTS FLAG_LIST)
  list(APPEND MLLVM_FLAGS "-mllvm" "${F}")
endforeach()
execute_process(
  COMMAND ${CLANG} -O2 -fpass-plugin=${PLUGIN} ${MLLVM_FLAGS}
          ${SOURCE} -o ${OBF_BIN}
  RESULT_VARIABLE OBF_COMPILE_RESULT
  ERROR_VARIABLE  OBF_COMPILE_ERR
)
if(NOT OBF_COMPILE_RESULT EQUAL 0)
  message(FATAL_ERROR "Obfuscated compile failed (${FLAGS}):\n${OBF_COMPILE_ERR}")
endif()

# ---- Run obfuscated binary --------------------------------------------------
execute_process(
  COMMAND ${OBF_BIN}
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
