# kagura-android-ndk.cmake
# Android NDK CMake integration for the kagura LLVM obfuscator.
#
# ─────────────────────────────────────────────────────────────────────────────
# Quick-start
# ─────────────────────────────────────────────────────────────────────────────
#
# 1. In your app/build.gradle point CMake at this file and set the plugin path:
#
#      android {
#        defaultConfig {
#          externalNativeBuild {
#            cmake {
#              arguments "-DKAGURA_PLUGIN_PATH=/path/to/KaguraObfuscator.so",
#                        "-DKAGURA_PROFILE=BALANCED"
#            }
#          }
#        }
#        externalNativeBuild {
#          cmake { path "CMakeLists.txt" }
#        }
#      }
#
# 2. In your native CMakeLists.txt:
#
#      cmake_minimum_required(VERSION 3.22)
#      project(mygame)
#
#      include(path/to/kagura-android-ndk.cmake)
#
#      kagura_android_config()          # validate env, set default flags
#
#      add_library(mynativelib SHARED src/native.cpp)
#      kagura_android_target(mynativelib)
#
# ─────────────────────────────────────────────────────────────────────────────
# CMake cache variables (override via -D on the cmake command line or in
# build.gradle's arguments block)
# ─────────────────────────────────────────────────────────────────────────────
#
#   KAGURA_PLUGIN_PATH   Path to KaguraObfuscator.so/.dylib (required)
#   KAGURA_PROFILE       Obfuscation profile: FAST | BALANCED | STRONG | CUSTOM
#                        (default: BALANCED)
#   KAGURA_RUNTIME_DIR   Directory containing il2cpp_protection.c and other
#                        runtime sources (default: auto-detected)
#
#   Fine-grained pass toggles (only respected when KAGURA_PROFILE=CUSTOM):
#     KAGURA_ENABLE_STR        String encryption           (default ON)
#     KAGURA_ENABLE_FLA        CFG flattening              (default ON)
#     KAGURA_ENABLE_BCF        Bogus control flow          (default OFF)
#     KAGURA_ENABLE_SUB        Instruction substitution    (default OFF)
#     KAGURA_ENABLE_CO         Constant obfuscation (MBA)  (default OFF)
#     KAGURA_ENABLE_JNI        JNI dynamic registration    (default ON)
#     KAGURA_ENABLE_ANTIDEBUG  Anti-debug / Anti-Frida     (default ON)
#     KAGURA_ENABLE_IL2CPP     IL2CPP runtime protection   (default OFF)
#     KAGURA_BCF_PROB          Bogus CF probability [0-100] (default 30)
#     KAGURA_BCF_ITER          Bogus CF iterations          (default 1)
#     KAGURA_SUB_ITER          Substitution iterations      (default 1)
#     KAGURA_SEED              PRNG seed (0 = system entropy)(default 0)
#     KAGURA_METRICS           Emit obfuscation metrics      (default OFF)

cmake_minimum_required(VERSION 3.22)

# ─────────────────────────────────────────────────────────────────────────────
# Internal: resolve this file's directory so helper paths work regardless
# of where the including CMakeLists.txt lives.
# ─────────────────────────────────────────────────────────────────────────────

get_filename_component(_KAGURA_NDK_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
get_filename_component(_KAGURA_NDK_DIR "${_KAGURA_NDK_DIR}" ABSOLUTE)

# Auto-detect the runtime source directory (two levels up from integration/android/)
if(NOT DEFINED KAGURA_RUNTIME_DIR)
  get_filename_component(_KAGURA_ROOT "${_KAGURA_NDK_DIR}/../.." ABSOLUTE)
  set(KAGURA_RUNTIME_DIR "${_KAGURA_ROOT}/runtime"
      CACHE PATH "Directory containing kagura runtime C sources")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Profile definitions
# ─────────────────────────────────────────────────────────────────────────────

# FAST    : low overhead — FLA + SUB only
# BALANCED: medium overhead — FAST + BCF + STR  (default)
# STRONG  : maximum protection — all passes enabled, BCF probability 50
# CUSTOM  : honour individual KAGURA_ENABLE_* variables as-is

set(KAGURA_PROFILE "BALANCED" CACHE STRING
    "Obfuscation profile: FAST | BALANCED | STRONG | CUSTOM")
set_property(CACHE KAGURA_PROFILE PROPERTY STRINGS FAST BALANCED STRONG CUSTOM)

# ─────────────────────────────────────────────────────────────────────────────
# Fine-grained pass options (defaults; profiles override these in
# kagura_android_config())
# ─────────────────────────────────────────────────────────────────────────────

option(KAGURA_ENABLE_STR       "String encryption"              ON)
option(KAGURA_ENABLE_FLA       "CFG flattening"                 ON)
option(KAGURA_ENABLE_BCF       "Bogus control flow"             OFF)
option(KAGURA_ENABLE_SUB       "Instruction substitution"       OFF)
option(KAGURA_ENABLE_CO        "Constant obfuscation (MBA)"     OFF)
option(KAGURA_ENABLE_JNI       "JNI dynamic registration"       ON)
option(KAGURA_ENABLE_ANTIDEBUG "Anti-debug / Anti-Frida"        ON)
option(KAGURA_ENABLE_IL2CPP    "IL2CPP runtime protection"      OFF)
option(KAGURA_METRICS          "Print obfuscation metrics"      OFF)

set(KAGURA_BCF_PROB 30  CACHE STRING "Bogus CF probability [0-100]")
set(KAGURA_BCF_ITER  1  CACHE STRING "Bogus CF iterations")
set(KAGURA_SUB_ITER  1  CACHE STRING "Substitution iterations")
set(KAGURA_SEED      0  CACHE STRING "PRNG seed (0 = system entropy)")

# ─────────────────────────────────────────────────────────────────────────────
# Internal: build the compile-options list from the current flag variables.
# Result is stored in OUT_VAR as a CMake list (semicolon-separated).
# ─────────────────────────────────────────────────────────────────────────────

function(_kagura_ndk_build_flags OUT_VAR)
  if(NOT EXISTS "${KAGURA_PLUGIN_PATH}")
    message(WARNING
      "[kagura] Plugin not found at ${KAGURA_PLUGIN_PATH} — obfuscation disabled")
    set(${OUT_VAR} "" PARENT_SCOPE)
    return()
  endif()

  set(_flags "SHELL:-fpass-plugin=${KAGURA_PLUGIN_PATH}")

  macro(_add_mllvm _flag)
    list(APPEND _flags "SHELL:-mllvm ${_flag}")
  endmacro()

  if(KAGURA_ENABLE_STR)
    _add_mllvm(-kagura-str)
  endif()
  if(KAGURA_ENABLE_FLA)
    _add_mllvm(-kagura-fla)
  endif()
  if(KAGURA_ENABLE_BCF)
    _add_mllvm(-kagura-bcf)
  endif()
  if(KAGURA_ENABLE_SUB)
    _add_mllvm(-kagura-sub)
  endif()
  if(KAGURA_ENABLE_CO)
    _add_mllvm(-kagura-co)
  endif()
  if(KAGURA_ENABLE_JNI)
    _add_mllvm(-kagura-jni)
  endif()
  if(KAGURA_ENABLE_ANTIDEBUG)
    _add_mllvm(-kagura-anti-debug)
  endif()
  if(KAGURA_METRICS)
    _add_mllvm(-kagura-metrics)
  endif()

  if(NOT KAGURA_BCF_PROB EQUAL 30)
    _add_mllvm("-kagura-bcf-prob=${KAGURA_BCF_PROB}")
  endif()
  if(NOT KAGURA_BCF_ITER EQUAL 1)
    _add_mllvm("-kagura-bcf-iter=${KAGURA_BCF_ITER}")
  endif()
  if(NOT KAGURA_SUB_ITER EQUAL 1)
    _add_mllvm("-kagura-sub-iter=${KAGURA_SUB_ITER}")
  endif()
  if(NOT KAGURA_SEED EQUAL 0)
    _add_mllvm("-kagura-seed=${KAGURA_SEED}")
  endif()

  set(${OUT_VAR} "${_flags}" PARENT_SCOPE)
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# Internal: emit ABI-specific compile options onto TARGET_NAME.
# Called from kagura_android_target() after the main flag set.
# ─────────────────────────────────────────────────────────────────────────────

function(_kagura_ndk_abi_flags TARGET_NAME)
  if(NOT ANDROID_ABI)
    return()
  endif()

  if(ANDROID_ABI STREQUAL "armeabi-v7a")
    # Thumb-2 interworking — ensure the compiler stays in Thumb mode for
    # the smaller code size that partially offsets BCF overhead.
    target_compile_options(${TARGET_NAME} PRIVATE
      -mthumb
      -mfpu=neon
    )
    # BCF adds opaque predicates that stress the branch predictor; cap the
    # iteration count at 1 for 32-bit ARM to limit size regression.
    if(KAGURA_ENABLE_BCF AND KAGURA_BCF_ITER GREATER 1)
      message(STATUS
        "[kagura] armeabi-v7a: capping BCF iterations at 1 to limit code size")
      target_compile_options(${TARGET_NAME} PRIVATE
        "SHELL:-mllvm -kagura-bcf-iter=1"
      )
    endif()

  elseif(ANDROID_ABI STREQUAL "arm64-v8a")
    # SVE / NEON hint so the compiler can vectorise helper code in the
    # runtime after inlining.  No kagura-specific cap needed on arm64.
    target_compile_options(${TARGET_NAME} PRIVATE
      -march=armv8-a
    )

  elseif(ANDROID_ABI STREQUAL "x86_64")
    # x86-64 Android (emulator, Chrome OS, some tablets).
    # CFG flattening adds indirect-branch overhead on x86; keep BCF off by
    # default for this ABI unless the caller explicitly opted in.
    if(KAGURA_ENABLE_BCF AND NOT _KAGURA_BCF_X86_64_OVERRIDE)
      message(STATUS
        "[kagura] x86_64: BCF enabled — consider KAGURA_BCF_PROB <= 20 "
        "to limit branch-misprediction overhead on this ABI")
    endif()

  elseif(ANDROID_ABI STREQUAL "x86")
    # 32-bit x86 (legacy emulator).  Disable BCF entirely; the indirect-
    # call overhead is severe and the ABI is rarely targeted in production.
    if(KAGURA_ENABLE_BCF)
      message(WARNING
        "[kagura] x86 ABI: BCF not recommended — code size/perf impact is "
        "disproportionate.  Set KAGURA_ENABLE_BCF=OFF to suppress this warning.")
    endif()
  endif()
endfunction()

# ═════════════════════════════════════════════════════════════════════════════
# Public API
# ═════════════════════════════════════════════════════════════════════════════

# ─────────────────────────────────────────────────────────────────────────────
# kagura_android_config()
#
# Validates the build environment and applies the selected KAGURA_PROFILE to
# the individual KAGURA_ENABLE_* flags.  Call once near the top of your root
# CMakeLists.txt, before any kagura_android_target() calls.
#
# Performs the following checks:
#   1. Warns if KAGURA_PLUGIN_PATH is not set or the file does not exist.
#   2. Checks the LLVM version embedded in the plugin filename/directory.
#   3. Applies profile presets to the KAGURA_ENABLE_* cache variables.
# ─────────────────────────────────────────────────────────────────────────────

function(kagura_android_config)
  # ---- Plugin path validation ----
  if(NOT DEFINED KAGURA_PLUGIN_PATH)
    # Fall back to a path relative to this file's location.
    if(ANDROID)
      set(KAGURA_PLUGIN_PATH
          "${_KAGURA_NDK_DIR}/../../build/lib/Transforms/KaguraObfuscator.so"
          CACHE PATH "Path to KaguraObfuscator plugin" FORCE)
    else()
      set(KAGURA_PLUGIN_PATH
          "${_KAGURA_NDK_DIR}/../../build/lib/Transforms/KaguraObfuscator.dylib"
          CACHE PATH "Path to KaguraObfuscator plugin" FORCE)
    endif()
  endif()

  get_filename_component(_kp "${KAGURA_PLUGIN_PATH}" ABSOLUTE)
  if(NOT EXISTS "${_kp}")
    message(WARNING
      "[kagura] Plugin not found: ${_kp}\n"
      "  Build the plugin first:  cmake --build <kagura-build-dir>\n"
      "  Then set -DKAGURA_PLUGIN_PATH=<path>")
  else()
    message(STATUS "[kagura] Plugin: ${_kp}")
  endif()

  # ---- LLVM version check ----
  # The plugin filename conventionally encodes the LLVM version, e.g.:
  #   KaguraObfuscator-llvm17.so
  # If found, verify it matches the NDK's clang major version.
  if(DEFINED CMAKE_C_COMPILER)
    execute_process(
      COMMAND "${CMAKE_C_COMPILER}" --version
      OUTPUT_VARIABLE _cc_ver OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    if(_cc_ver MATCHES "clang version ([0-9]+)")
      set(_clang_major "${CMAKE_MATCH_1}")
      if(KAGURA_PLUGIN_PATH MATCHES "llvm([0-9]+)")
        set(_plugin_llvm "${CMAKE_MATCH_1}")
        if(NOT _clang_major STREQUAL _plugin_llvm)
          message(WARNING
            "[kagura] LLVM version mismatch: clang=${_clang_major}, "
            "plugin built for LLVM ${_plugin_llvm}.  "
            "The plugin may crash or silently miscompile.")
        else()
          message(STATUS "[kagura] LLVM version: ${_clang_major} (matched)")
        endif()
      endif()
    endif()
  endif()

  # ---- Apply profile presets ----
  # Profiles set the CACHE variables so they are visible to all subsequent
  # CMakeLists.txt files processed after this call.

  if(KAGURA_PROFILE STREQUAL "FAST")
    # Lowest overhead: FLA + SUB.  No BCF, no string encryption.
    set(KAGURA_ENABLE_STR       OFF CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_FLA       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_BCF       OFF CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_SUB       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_CO        OFF CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_JNI       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_ANTIDEBUG ON  CACHE BOOL "" FORCE)
    set(KAGURA_BCF_PROB         20  CACHE STRING "" FORCE)
    message(STATUS "[kagura] Profile: FAST  (fla + sub)")

  elseif(KAGURA_PROFILE STREQUAL "BALANCED")
    # Medium overhead: FLA + BCF + STR + SUB.
    set(KAGURA_ENABLE_STR       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_FLA       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_BCF       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_SUB       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_CO        OFF CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_JNI       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_ANTIDEBUG ON  CACHE BOOL "" FORCE)
    set(KAGURA_BCF_PROB         30  CACHE STRING "" FORCE)
    message(STATUS "[kagura] Profile: BALANCED  (fla + bcf + str + sub)")

  elseif(KAGURA_PROFILE STREQUAL "STRONG")
    # Maximum protection: all passes, higher BCF probability.
    set(KAGURA_ENABLE_STR       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_FLA       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_BCF       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_SUB       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_CO        ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_JNI       ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_ANTIDEBUG ON  CACHE BOOL "" FORCE)
    set(KAGURA_ENABLE_IL2CPP    ON  CACHE BOOL "" FORCE)
    set(KAGURA_BCF_PROB         50  CACHE STRING "" FORCE)
    set(KAGURA_BCF_ITER         2   CACHE STRING "" FORCE)
    message(STATUS "[kagura] Profile: STRONG  (all passes, bcf-prob=50)")

  elseif(KAGURA_PROFILE STREQUAL "CUSTOM")
    message(STATUS
      "[kagura] Profile: CUSTOM  (using individual KAGURA_ENABLE_* variables)")

  else()
    message(WARNING
      "[kagura] Unknown profile '${KAGURA_PROFILE}'. "
      "Falling back to BALANCED.  Valid values: FAST BALANCED STRONG CUSTOM")
    set(KAGURA_PROFILE "BALANCED" CACHE STRING "" FORCE)
  endif()
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# kagura_android_target(target_name)
#
# Applies the kagura LLVM pass plugin and all configured flags to the given
# CMake target.  Also links kagura_runtime if the target has been built via
# kagura_android_runtime_target().
#
# Must be called after add_library() / add_executable() for target_name.
# kagura_android_config() should be called before the first invocation of
# this function.
# ─────────────────────────────────────────────────────────────────────────────

function(kagura_android_target TARGET_NAME)
  if(NOT TARGET ${TARGET_NAME})
    message(FATAL_ERROR
      "[kagura] kagura_android_target: '${TARGET_NAME}' is not a CMake target. "
      "Call add_library() or add_executable() first.")
  endif()

  # Build the compile-options list.
  _kagura_ndk_build_flags(_KAGURA_FLAGS)

  if(_KAGURA_FLAGS)
    target_compile_options(${TARGET_NAME} PRIVATE ${_KAGURA_FLAGS})
    message(STATUS "[kagura] Obfuscation applied to target: ${TARGET_NAME}")
  endif()

  # ABI-specific overrides / warnings.
  _kagura_ndk_abi_flags(${TARGET_NAME})

  # Link the runtime if it has been registered via kagura_android_runtime_target().
  if(TARGET kagura_runtime)
    target_link_libraries(${TARGET_NAME} PRIVATE kagura_runtime)
    message(STATUS "[kagura] Linked kagura_runtime to target: ${TARGET_NAME}")
  endif()

  # Pass the IL2CPP protection flag when the runtime target includes
  # il2cpp_protection.c and the profile requests it.
  if(KAGURA_ENABLE_IL2CPP AND TARGET kagura_runtime)
    target_compile_definitions(${TARGET_NAME} PRIVATE KAGURA_IL2CPP_PROTECTION=1)
  endif()
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# kagura_android_runtime_target(target_name)
#
# Builds the kagura_runtime static library from the runtime C sources and
# registers it under the canonical name "kagura_runtime" so that
# kagura_android_target() can automatically link it.
#
# Parameters:
#   target_name   Name for the runtime static library target.  Pass
#                 "kagura_runtime" to use the conventional name; any other
#                 name creates an ALIAS so both names work.
#
# Example:
#   kagura_android_runtime_target(kagura_runtime)
#   kagura_android_target(mynativelib)   # auto-links kagura_runtime
# ─────────────────────────────────────────────────────────────────────────────

function(kagura_android_runtime_target TARGET_NAME)
  if(TARGET kagura_runtime)
    message(STATUS
      "[kagura] kagura_runtime already defined — skipping second registration")
    return()
  endif()

  if(NOT EXISTS "${KAGURA_RUNTIME_DIR}")
    message(WARNING
      "[kagura] Runtime source directory not found: ${KAGURA_RUNTIME_DIR}\n"
      "  Set -DKAGURA_RUNTIME_DIR=<path> to the directory containing "
      "anti_debug.c, jailbreak_detection.c, aes.c, etc.")
    return()
  endif()

  # Collect runtime sources that always compile.
  set(_runtime_sources
    "${KAGURA_RUNTIME_DIR}/anti_debug.c"
    "${KAGURA_RUNTIME_DIR}/aes.c"
    "${KAGURA_RUNTIME_DIR}/jailbreak_detection.c"
    "${KAGURA_RUNTIME_DIR}/vm_interpreter.c"
  )

  # Optionally include the IL2CPP protection module.
  if(KAGURA_ENABLE_IL2CPP)
    set(_il2cpp_src "${KAGURA_RUNTIME_DIR}/il2cpp_protection.c")
    if(EXISTS "${_il2cpp_src}")
      list(APPEND _runtime_sources "${_il2cpp_src}")
    else()
      message(WARNING
        "[kagura] KAGURA_ENABLE_IL2CPP=ON but ${_il2cpp_src} not found")
    endif()
  endif()

  add_library(${TARGET_NAME} STATIC ${_runtime_sources})

  # Export headers from the project's include directory.
  get_filename_component(_kagura_include
    "${KAGURA_RUNTIME_DIR}/../include" ABSOLUTE)
  if(EXISTS "${_kagura_include}")
    target_include_directories(${TARGET_NAME} PUBLIC "${_kagura_include}")
  endif()

  set_target_properties(${TARGET_NAME} PROPERTIES
    C_STANDARD              11
    C_STANDARD_REQUIRED     ON
    POSITION_INDEPENDENT_CODE ON
  )

  target_compile_options(${TARGET_NAME} PRIVATE
    -Wall -Wextra -Wno-unused-parameter
  )

  # Create the canonical alias if the caller chose a different name.
  if(NOT TARGET_NAME STREQUAL "kagura_runtime")
    add_library(kagura_runtime ALIAS ${TARGET_NAME})
  endif()

  message(STATUS "[kagura] Runtime target '${TARGET_NAME}' configured")
  message(STATUS "[kagura] Runtime sources: ${_runtime_sources}")
endfunction()
