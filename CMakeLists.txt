cmake_minimum_required(VERSION 3.20)
project(kagura VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- 4.6.8 Build cache support -------------------------------------------
# Enable ccache / sccache if available. Set KAGURA_USE_CACHE=OFF to disable.
option(KAGURA_USE_CACHE "Use compiler cache (ccache/sccache) if available" ON)
if(KAGURA_USE_CACHE)
  # Prefer sccache (cloud-friendly) over ccache.
  find_program(SCCACHE_PROGRAM sccache)
  find_program(CCACHE_PROGRAM  ccache)
  if(SCCACHE_PROGRAM)
    message(STATUS "[kagura] Build cache: sccache (${SCCACHE_PROGRAM})")
    set(CMAKE_C_COMPILER_LAUNCHER   "${SCCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
  elseif(CCACHE_PROGRAM)
    message(STATUS "[kagura] Build cache: ccache (${CCACHE_PROGRAM})")
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  else()
    message(STATUS "[kagura] Build cache: not available (install ccache or sccache)")
  endif()
endif()
# --------------------------------------------------------------------------

# LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Pull in LLVM's CMake helpers (provides add_llvm_pass_plugin etc.)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# Kagura includes
include_directories(${CMAKE_SOURCE_DIR}/include)

# ---- 4.6.7 Incremental build support ------------------------------------
# kagura-opt and the pass plugin both support incremental builds naturally
# via CMake's dependency tracking.  The following options control additional
# incremental-build optimisations:
#
#   KAGURA_UNITY_BUILD=ON  — merge pass sources into unity build groups so
#     that incremental rebuilds recompile fewer TUs when a header changes.
#     Speeds up full rebuilds ~2x but slows incremental single-file changes.
#
#   KAGURA_PCH=ON  — enable precompiled header for LLVM headers (llvm/IR/*.h)
#     which account for most of the include overhead per TU.
option(KAGURA_UNITY_BUILD "Enable CMake unity build for the obfuscator plugin" OFF)
option(KAGURA_PCH "Enable precompiled header for LLVM includes" ON)

if(KAGURA_PCH)
  set(CMAKE_DISABLE_PRECOMPILE_HEADERS OFF)
endif()
# --------------------------------------------------------------------------

# ---- 4.1.3 Legacy pass manager compatibility ----------------------------
# When KAGURA_LEGACY_PM=ON, LegacyPlugin.cpp is compiled into the shared
# library and registers all kagura passes with LLVM 14-16's legacy PM via
# PassManagerBuilder extension points and RegisterPass<>.
# Requires LLVM_VERSION_MAJOR <= 16; newer LLVMs removed the legacy PM APIs.
option(KAGURA_LEGACY_PM "Build legacy pass manager shims (LLVM 14-16)" OFF)
if(KAGURA_LEGACY_PM)
  if(LLVM_VERSION_MAJOR GREATER 16)
    message(WARNING "[kagura] KAGURA_LEGACY_PM=ON but LLVM ${LLVM_VERSION_MAJOR} "
                    "has removed legacy PM APIs — disabling.")
    set(KAGURA_LEGACY_PM OFF)
  else()
    message(STATUS "[kagura] Legacy PM shims enabled (LLVM ${LLVM_VERSION_MAJOR})")
  endif()
endif()
# --------------------------------------------------------------------------

# ---- 4.1.5 Bitcode input support -----------------------------------------
# When KAGURA_BITCODE_TOOLS=ON, build a small standalone tool (kagura-opt)
# that wraps `opt` and auto-loads the kagura plugin, so users can run kagura
# passes on .bc / .ll files without needing a full clang invocation.
option(KAGURA_BITCODE_TOOLS "Build kagura-opt bitcode processing tool" OFF)
# --------------------------------------------------------------------------

# Passes (loaded as plugin via -fpass-plugin)
add_subdirectory(lib)

# Runtime library linked into target app
add_subdirectory(runtime)

# Tests
option(KAGURA_BUILD_TESTS "Build tests" ON)
if(KAGURA_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# Bitcode tools (kagura-opt)
if(KAGURA_BITCODE_TOOLS)
  add_subdirectory(tools)
endif()
