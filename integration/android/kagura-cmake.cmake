# kagura-cmake.cmake
# CMake snippet for Android NDK projects.
#
# Usage — in your app's CMakeLists.txt:
#
#   include(${CMAKE_SOURCE_DIR}/../kagura/integration/android/kagura-cmake.cmake)
#   kagura_target(my_native_lib)
#
# Or to apply globally:
#
#   include(${CMAKE_SOURCE_DIR}/../kagura/integration/android/kagura-cmake.cmake)
#   kagura_apply_global()

cmake_minimum_required(VERSION 3.18)

# ── Locate the plugin ──────────────────────────────────────────────────────────

if(NOT DEFINED KAGURA_PLUGIN_PATH)
  # Search relative to this file's directory
  get_filename_component(_KAGURA_INTEG_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
  set(KAGURA_PLUGIN_PATH "${_KAGURA_INTEG_DIR}/../../build/lib/Transforms/KaguraObfuscator.dylib")
endif()

# Android: the plugin is a .so
if(ANDROID AND NOT EXISTS "${KAGURA_PLUGIN_PATH}")
  string(REPLACE ".dylib" ".so" KAGURA_PLUGIN_PATH "${KAGURA_PLUGIN_PATH}")
endif()

# ── Configuration variables (override on cmake command line) ───────────────────
option(KAGURA_ENABLE_STR       "String encryption"              ON)
option(KAGURA_ENABLE_FLA       "CFG flattening"                 ON)
option(KAGURA_ENABLE_BCF       "Bogus control flow"             ON)
option(KAGURA_ENABLE_SUB       "Instruction substitution"       ON)
option(KAGURA_ENABLE_CO        "Constant obfuscation (MBA)"     OFF)
option(KAGURA_ENABLE_JNI       "JNI dynamic registration"       ON)
option(KAGURA_ENABLE_ANTIDEBUG "Anti-debug / Anti-Frida"        ON)
option(KAGURA_METRICS          "Print obfuscation metrics"      OFF)

set(KAGURA_BCF_PROB 30 CACHE STRING "Bogus CF probability [0-100]")
set(KAGURA_BCF_ITER  1 CACHE STRING "Bogus CF iterations")
set(KAGURA_SUB_ITER  1 CACHE STRING "Substitution iterations")
set(KAGURA_SEED      0 CACHE STRING "PRNG seed (0 = system entropy)")

# ── Build flag list ────────────────────────────────────────────────────────────

function(_kagura_build_flags OUT_VAR)
  if(NOT EXISTS "${KAGURA_PLUGIN_PATH}")
    message(WARNING "[kagura] Plugin not found at ${KAGURA_PLUGIN_PATH} — obfuscation disabled")
    set(${OUT_VAR} "" PARENT_SCOPE)
    return()
  endif()

  # Build as a CMake list (semicolon-separated) for target_compile_options
  # Build flags as a single SHELL: string so CMake passes -mllvm <flag> as pairs
  set(_flags "SHELL:-fpass-plugin=${KAGURA_PLUGIN_PATH}")

  macro(add_mllvm_flag flag)
    list(APPEND _flags "SHELL:-mllvm ${flag}")
  endmacro()

  if(KAGURA_ENABLE_STR)
    add_mllvm_flag(-kagura-str)
  endif()
  if(KAGURA_ENABLE_FLA)
    add_mllvm_flag(-kagura-fla)
  endif()
  if(KAGURA_ENABLE_BCF)
    add_mllvm_flag(-kagura-bcf)
  endif()
  if(KAGURA_ENABLE_SUB)
    add_mllvm_flag(-kagura-sub)
  endif()
  if(KAGURA_ENABLE_CO)
    add_mllvm_flag(-kagura-co)
  endif()
  if(KAGURA_ENABLE_JNI)
    add_mllvm_flag(-kagura-jni)
  endif()
  if(KAGURA_ENABLE_ANTIDEBUG)
    add_mllvm_flag(-kagura-anti-debug)
  endif()
  if(KAGURA_METRICS)
    add_mllvm_flag(-kagura-metrics)
  endif()

  if(NOT KAGURA_BCF_PROB EQUAL 30)
    add_mllvm_flag("-kagura-bcf-prob=${KAGURA_BCF_PROB}")
  endif()
  if(NOT KAGURA_BCF_ITER EQUAL 1)
    add_mllvm_flag("-kagura-bcf-iter=${KAGURA_BCF_ITER}")
  endif()
  if(NOT KAGURA_SUB_ITER EQUAL 1)
    add_mllvm_flag("-kagura-sub-iter=${KAGURA_SUB_ITER}")
  endif()
  if(NOT KAGURA_SEED EQUAL 0)
    add_mllvm_flag("-kagura-seed=${KAGURA_SEED}")
  endif()

  set(${OUT_VAR} "${_flags}" PARENT_SCOPE)
endfunction()

# ── Public API ─────────────────────────────────────────────────────────────────

# Apply kagura flags to a specific target
function(kagura_target TARGET_NAME)
  _kagura_build_flags(_KAGURA_FLAGS)
  if(_KAGURA_FLAGS)
    # _KAGURA_FLAGS is already a CMake list (semicolon-separated)
    target_compile_options(${TARGET_NAME} PRIVATE ${_KAGURA_FLAGS})
    message(STATUS "[kagura] Obfuscation applied to target: ${TARGET_NAME}")
  endif()
endfunction()

# Apply kagura flags to all subsequent targets in this CMakeLists.txt
function(kagura_apply_global)
  _kagura_build_flags(_KAGURA_FLAGS)
  if(_KAGURA_FLAGS)
    add_compile_options(${_KAGURA_FLAGS})
    message(STATUS "[kagura] Obfuscation applied globally")
  endif()
endfunction()
