# run_pass_test.cmake — Two-stage pass test runner
# Usage: cmake -DCLANG=... -DOPT=... -DPLUGIN=... -DPASSES=... -DSOURCE=... -P run_pass_test.cmake
#
# 1. Compile C source to LLVM IR with clang
# 2. Run opt with the pass plugin and specified passes

cmake_minimum_required(VERSION 3.20)

# Generate a temp file for IR
string(RANDOM LENGTH 8 ALPHABET abcdefghijklmnopqrstuvwxyz RAND_SUFFIX)
set(IR_FILE "${CMAKE_CURRENT_BINARY_DIR}/kagura_test_${RAND_SUFFIX}.ll")

# Step 1: C → LLVM IR
execute_process(
  COMMAND ${CLANG} -O1 -emit-llvm -S -o ${IR_FILE} ${SOURCE}
  RESULT_VARIABLE CLANG_RESULT
  ERROR_VARIABLE CLANG_ERROR
)

if(NOT CLANG_RESULT EQUAL 0)
  message(FATAL_ERROR "clang failed:\n${CLANG_ERROR}")
endif()

# Step 2: opt with pass plugin
execute_process(
  COMMAND ${OPT} --load-pass-plugin=${PLUGIN} -passes=${PASSES} -S -o /dev/null ${IR_FILE}
  RESULT_VARIABLE OPT_RESULT
  ERROR_VARIABLE OPT_ERROR
)

# Cleanup
file(REMOVE ${IR_FILE})

if(NOT OPT_RESULT EQUAL 0)
  message(FATAL_ERROR "opt failed:\n${OPT_ERROR}")
endif()
