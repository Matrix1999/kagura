# kagura_unity_config.cmake
#
# CMake helper for Unity IL2CPP builds.
#
# Unity's IL2CPP backend generates a CMakeLists.txt and invokes cmake/ninja
# internally.  This file can be included from that generated CMakeLists.txt
# (or from the Gradle externalNativeBuild CMake arguments) to inject kagura
# obfuscation flags into every C/C++ compile command.
#
# Usage — from CMake command line or Gradle:
#   cmake -DCMAKE_TOOLCHAIN_FILE=path/to/android.toolchain.cmake \
#         -DKAGURA_CMAKE_DIR=/path/to/kagura/integration/cmake \
#         -C /path/to/kagura/integration/unity/kagura_unity_config.cmake \
#         ...
#
# Or include from the generated CMakeLists.txt:
#   include("/path/to/kagura/integration/unity/kagura_unity_config.cmake")
#
# ── Required variables ────────────────────────────────────────────────────────
#   KAGURA_PLUGIN_PATH   Path to KaguraObfuscator.dylib / .so
#   KAGURA_RUNTIME_LIB   Path to libkagura_runtime.a
#
# ── Optional variables (all default to ON) ────────────────────────────────────
#   KAGURA_ENABLE_STR         String encryption
#   KAGURA_ENABLE_FLA         CFG flattening
#   KAGURA_ENABLE_BCF         Bogus control flow
#   KAGURA_ENABLE_SUB         Instruction substitution
#   KAGURA_ENABLE_IBR         Indirect branch
#   KAGURA_ENABLE_BBR         BB reordering
#   KAGURA_ENABLE_SV          Symbol visibility
#   KAGURA_ENABLE_ANTI_DEBUG  Anti-debug
#   KAGURA_ENABLE_TAMPER      Anti-tamper
#   KAGURA_BCF_PROB           Bogus CF probability [0-100] (default: 30)
#   KAGURA_SEED               PRNG seed (default: 0 = entropy)
# ─────────────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.20)

# ── Locate plugin if not set ──────────────────────────────────────────────────
if(NOT KAGURA_PLUGIN_PATH)
  find_file(KAGURA_PLUGIN_PATH
    NAMES KaguraObfuscator.dylib KaguraObfuscator.so
    HINTS
      "${CMAKE_CURRENT_LIST_DIR}/../../build/lib/Transforms"
      "${CMAKE_SOURCE_DIR}/../kagura/build/lib/Transforms"
    DOC "Path to KaguraObfuscator plugin"
  )
endif()

if(NOT KAGURA_PLUGIN_PATH OR NOT EXISTS "${KAGURA_PLUGIN_PATH}")
  message(WARNING "[kagura] Plugin not found — obfuscation disabled. "
                  "Set KAGURA_PLUGIN_PATH to KaguraObfuscator.dylib/.so")
  return()
endif()

# ── Locate runtime lib ────────────────────────────────────────────────────────
if(NOT KAGURA_RUNTIME_LIB)
  find_file(KAGURA_RUNTIME_LIB
    NAMES libkagura_runtime.a
    HINTS
      "${CMAKE_CURRENT_LIST_DIR}/../../build/runtime"
      "${CMAKE_SOURCE_DIR}/../kagura/build/runtime"
    DOC "Path to libkagura_runtime.a"
  )
endif()

# ── Default pass settings ─────────────────────────────────────────────────────
option(KAGURA_ENABLE_STR        "kagura: string encryption"     ON)
option(KAGURA_ENABLE_FLA        "kagura: CFG flattening"        ON)
option(KAGURA_ENABLE_BCF        "kagura: bogus control flow"    ON)
option(KAGURA_ENABLE_SUB        "kagura: substitution"          ON)
option(KAGURA_ENABLE_IBR        "kagura: indirect branch"       ON)
option(KAGURA_ENABLE_BBR        "kagura: BB reordering"         ON)
option(KAGURA_ENABLE_BBS        "kagura: BB splitting"          OFF)
option(KAGURA_ENABLE_DCI        "kagura: dead code insertion"   OFF)
option(KAGURA_ENABLE_SV         "kagura: symbol visibility"     ON)
option(KAGURA_ENABLE_ANTI_DEBUG "kagura: anti-debug"            ON)
option(KAGURA_ENABLE_TAMPER     "kagura: anti-tamper"           ON)
option(KAGURA_ENABLE_GENC       "kagura: global encryption"     OFF)
option(KAGURA_ENABLE_VM         "kagura: VM obfuscation"        OFF)
option(KAGURA_ENABLE_CO         "kagura: constant obfuscation"  OFF)

set(KAGURA_BCF_PROB "30" CACHE STRING "kagura: bogus CF probability [0-100]")
set(KAGURA_SEED     "0"  CACHE STRING "kagura: PRNG seed (0 = entropy)")

# ── Build flag string ─────────────────────────────────────────────────────────
set(_KAGURA_FLAGS "-fpass-plugin=${KAGURA_PLUGIN_PATH}")

macro(_kagura_add_flag _enabled _flag)
  if(${_enabled})
    string(APPEND _KAGURA_FLAGS " -mllvm ${_flag}")
  endif()
endmacro()

_kagura_add_flag(KAGURA_ENABLE_STR        "-kagura-str")
_kagura_add_flag(KAGURA_ENABLE_FLA        "-kagura-fla")
_kagura_add_flag(KAGURA_ENABLE_BCF        "-kagura-bcf")
_kagura_add_flag(KAGURA_ENABLE_SUB        "-kagura-sub")
_kagura_add_flag(KAGURA_ENABLE_CO         "-kagura-co")
_kagura_add_flag(KAGURA_ENABLE_IBR        "-kagura-ibr")
_kagura_add_flag(KAGURA_ENABLE_BBR        "-kagura-bbr")
_kagura_add_flag(KAGURA_ENABLE_BBS        "-kagura-bbs")
_kagura_add_flag(KAGURA_ENABLE_DCI        "-kagura-dci")
_kagura_add_flag(KAGURA_ENABLE_SV         "-kagura-sv")
_kagura_add_flag(KAGURA_ENABLE_ANTI_DEBUG "-kagura-anti-debug")
_kagura_add_flag(KAGURA_ENABLE_TAMPER     "-kagura-tamper")
_kagura_add_flag(KAGURA_ENABLE_GENC       "-kagura-genc")
_kagura_add_flag(KAGURA_ENABLE_VM         "-kagura-vm")

if(KAGURA_ENABLE_BCF AND NOT KAGURA_BCF_PROB STREQUAL "30")
  string(APPEND _KAGURA_FLAGS " -mllvm -kagura-bcf-prob=${KAGURA_BCF_PROB}")
endif()
if(NOT KAGURA_SEED STREQUAL "0")
  string(APPEND _KAGURA_FLAGS " -mllvm -kagura-seed=${KAGURA_SEED}")
endif()

# ── Inject into global compile options ────────────────────────────────────────
# These apply to every target defined after this include().
add_compile_options("SHELL:${_KAGURA_FLAGS}")

# ── Link runtime library ──────────────────────────────────────────────────────
if(KAGURA_RUNTIME_LIB AND EXISTS "${KAGURA_RUNTIME_LIB}")
  # Create an imported static library target so targets can link it by name.
  if(NOT TARGET kagura_runtime)
    add_library(kagura_runtime STATIC IMPORTED GLOBAL)
    set_target_properties(kagura_runtime PROPERTIES
      IMPORTED_LOCATION "${KAGURA_RUNTIME_LIB}"
    )
  endif()
  # Append to CMAKE_EXE_LINKER_FLAGS so IL2CPP's final link step picks it up.
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} ${KAGURA_RUNTIME_LIB}")
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} ${KAGURA_RUNTIME_LIB}")
else()
  message(WARNING "[kagura] Runtime lib not found — passes requiring runtime "
                  "support (AES, VM, AntiDebug, AntiTamper) will fail to link. "
                  "Set KAGURA_RUNTIME_LIB.")
endif()

message(STATUS "[kagura] Unity IL2CPP obfuscation enabled")
message(STATUS "[kagura]   Plugin : ${KAGURA_PLUGIN_PATH}")
message(STATUS "[kagura]   Runtime: ${KAGURA_RUNTIME_LIB}")
message(STATUS "[kagura]   Flags  : ${_KAGURA_FLAGS}")
