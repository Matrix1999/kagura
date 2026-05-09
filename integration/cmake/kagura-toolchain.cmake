# kagura-toolchain.cmake
#
# Generic CMake toolchain file for kagura obfuscation.
# Works with any CMake project (Cocos2d-x, Godot GDNative, custom engines).
#
# Usage
# -----
# Pass this file as the toolchain:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/kagura/integration/cmake/kagura-toolchain.cmake \
#         -DKAGURA_PLUGIN_PATH=/path/to/KaguraObfuscator.dylib \
#         -B build -S .
#
# If you already have a toolchain file (e.g. Android NDK toolchain), chain
# them by setting KAGURA_CHAIN_TOOLCHAIN:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/kagura/integration/cmake/kagura-toolchain.cmake \
#         -DKAGURA_CHAIN_TOOLCHAIN=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
#         -DANDROID_ABI=arm64-v8a \
#         -DKAGURA_PLUGIN_PATH=/path/to/KaguraObfuscator.so \
#         -B build -S .
#
# ── Required variables ────────────────────────────────────────────────────────
#   KAGURA_PLUGIN_PATH    Path to KaguraObfuscator.dylib / .so
#
# ── Optional variables ────────────────────────────────────────────────────────
#   KAGURA_RUNTIME_LIB    Path to libkagura_runtime.a
#   KAGURA_CHAIN_TOOLCHAIN  Another toolchain file to include first
#   KAGURA_PROFILE        Obfuscation profile: FAST | BALANCED | STRONG | OFF
#                         (default: BALANCED)
#   KAGURA_SEED           PRNG seed (default: 0 = entropy)
#   KAGURA_BCF_PROB       Bogus CF probability (default: 30)
#
# ── Profiles ──────────────────────────────────────────────────────────────────
#   FAST      str, sv, anti-debug
#   BALANCED  str, fla, bcf, sub, ibr, bbr, sv, anti-debug, tamper  (default)
#   STRONG    all passes including co, genc, bbs, dci, vm
#   OFF       no obfuscation (toolchain still chains correctly)
# ─────────────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.20)

# ── Chain an existing toolchain ───────────────────────────────────────────────
if(KAGURA_CHAIN_TOOLCHAIN AND NOT _KAGURA_CHAINED)
  set(_KAGURA_CHAINED TRUE)
  include("${KAGURA_CHAIN_TOOLCHAIN}")
endif()

# ── Skip if obfuscation is disabled ──────────────────────────────────────────
if(NOT DEFINED KAGURA_PROFILE)
  set(KAGURA_PROFILE "BALANCED")
endif()

if(KAGURA_PROFILE STREQUAL "OFF")
  return()
endif()

# ── Locate plugin ─────────────────────────────────────────────────────────────
if(NOT KAGURA_PLUGIN_PATH)
  # Try common relative locations
  foreach(_candidate
      "${CMAKE_CURRENT_LIST_DIR}/../../build/lib/Transforms/KaguraObfuscator.dylib"
      "${CMAKE_CURRENT_LIST_DIR}/../../build/lib/Transforms/KaguraObfuscator.so"
  )
    if(EXISTS "${_candidate}")
      set(KAGURA_PLUGIN_PATH "${_candidate}")
      break()
    endif()
  endforeach()
endif()

if(NOT KAGURA_PLUGIN_PATH OR NOT EXISTS "${KAGURA_PLUGIN_PATH}")
  message(WARNING
    "[kagura] Plugin not found — obfuscation disabled.\n"
    "  Set -DKAGURA_PLUGIN_PATH=/path/to/KaguraObfuscator.dylib")
  return()
endif()

# ── Build flag list from profile ──────────────────────────────────────────────
set(_KAGURA_FLAGS "-fpass-plugin=${KAGURA_PLUGIN_PATH}")

# Helper macro: append -mllvm <flag> pair
macro(_kf _flag)
  string(APPEND _KAGURA_FLAGS " -mllvm ${_flag}")
endmacro()

if(KAGURA_PROFILE STREQUAL "FAST")
  _kf(-kagura-str)
  _kf(-kagura-sv)
  _kf(-kagura-anti-debug)

elseif(KAGURA_PROFILE STREQUAL "BALANCED")
  _kf(-kagura-str)
  _kf(-kagura-fla)
  _kf(-kagura-bcf)
  _kf(-kagura-sub)
  _kf(-kagura-ibr)
  _kf(-kagura-bbr)
  _kf(-kagura-sv)
  _kf(-kagura-anti-debug)
  _kf(-kagura-tamper)

elseif(KAGURA_PROFILE STREQUAL "STRONG")
  _kf(-kagura-str)
  _kf(-kagura-fla)
  _kf(-kagura-bcf)
  _kf(-kagura-sub)
  _kf(-kagura-co)
  _kf(-kagura-ibr)
  _kf(-kagura-bbr)
  _kf(-kagura-bbs)
  _kf(-kagura-dci)
  _kf(-kagura-sv)
  _kf(-kagura-genc)
  _kf(-kagura-anti-debug)
  _kf(-kagura-tamper)
  _kf(-kagura-vm)

else()
  message(WARNING "[kagura] Unknown KAGURA_PROFILE '${KAGURA_PROFILE}' — using BALANCED")
  _kf(-kagura-str)
  _kf(-kagura-fla)
  _kf(-kagura-bcf)
  _kf(-kagura-sub)
  _kf(-kagura-ibr)
  _kf(-kagura-bbr)
  _kf(-kagura-sv)
  _kf(-kagura-anti-debug)
  _kf(-kagura-tamper)
endif()

# Tuning options
if(DEFINED KAGURA_BCF_PROB AND NOT KAGURA_BCF_PROB EQUAL 30)
  _kf(-kagura-bcf-prob=${KAGURA_BCF_PROB})
endif()
if(DEFINED KAGURA_SEED AND NOT KAGURA_SEED EQUAL 0)
  _kf(-kagura-seed=${KAGURA_SEED})
endif()

# ── Inject into compiler flags ────────────────────────────────────────────────
# Use INIT variables so they take effect before project() sees them,
# and use CACHE so they propagate to sub-projects.
set(CMAKE_C_FLAGS_INIT   "${CMAKE_C_FLAGS_INIT}   ${_KAGURA_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} ${_KAGURA_FLAGS}")

# ── Runtime library ───────────────────────────────────────────────────────────
if(NOT KAGURA_RUNTIME_LIB)
  foreach(_candidate
      "${CMAKE_CURRENT_LIST_DIR}/../../build/runtime/libkagura_runtime.a"
  )
    if(EXISTS "${_candidate}")
      set(KAGURA_RUNTIME_LIB "${_candidate}")
      break()
    endif()
  endforeach()
endif()

if(KAGURA_RUNTIME_LIB AND EXISTS "${KAGURA_RUNTIME_LIB}")
  set(CMAKE_EXE_LINKER_FLAGS_INIT
      "${CMAKE_EXE_LINKER_FLAGS_INIT} ${KAGURA_RUNTIME_LIB}")
  set(CMAKE_SHARED_LINKER_FLAGS_INIT
      "${CMAKE_SHARED_LINKER_FLAGS_INIT} ${KAGURA_RUNTIME_LIB}")
else()
  message(WARNING
    "[kagura] libkagura_runtime.a not found. "
    "Set -DKAGURA_RUNTIME_LIB=/path/to/libkagura_runtime.a")
endif()

message(STATUS "[kagura] Toolchain: profile=${KAGURA_PROFILE}")
message(STATUS "[kagura]   Plugin : ${KAGURA_PLUGIN_PATH}")
message(STATUS "[kagura]   Runtime: ${KAGURA_RUNTIME_LIB}")
