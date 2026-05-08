cmake_minimum_required(VERSION 3.20)
project(kagura VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
