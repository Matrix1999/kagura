cmake_minimum_required(VERSION 3.20)
project(kagura_sample_basic C)

# ── Locate kagura plugin ──────────────────────────────────────────────────────
# Pass -DKAGURA_DIR=/path/to/kagura on the cmake command line, or set the
# KAGURA_DIR environment variable.  Falls back to a sibling checkout.
if(NOT DEFINED KAGURA_DIR)
  set(KAGURA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH
      "Root of the kagura repository")
endif()

set(KAGURA_PLUGIN "${KAGURA_DIR}/build/lib/Transforms/KaguraObfuscator.dylib"
    CACHE FILEPATH "Path to KaguraObfuscator.dylib")

# ── Sample target ─────────────────────────────────────────────────────────────
add_executable(kagura_sample_basic main.c)

target_compile_options(kagura_sample_basic PRIVATE
  -O1   # Optimise before obfuscation so LLVM dead-code-eliminates first

  # Load the kagura pass plugin
  -fpass-plugin=${KAGURA_PLUGIN}

  # Control-flow passes
  -mllvm -kagura-fla
  -mllvm -kagura-bcf
  -mllvm -kagura-bcf-prob=30

  # Data passes
  -mllvm -kagura-str
  -mllvm -kagura-sub
  -mllvm -kagura-co

  # Anti-analysis
  -mllvm -kagura-ibr
  -mllvm -kagura-lt
)

# ── Optional: enable anti-debug runtime path ─────────────────────────────────
option(KAGURA_SAMPLE_ANTIDEB "Enable anti-debug checks in the sample" OFF)
if(KAGURA_SAMPLE_ANTIDEB)
  target_compile_definitions(kagura_sample_basic PRIVATE KAGURA_ENABLE_ANTIDEB=1)
  target_link_libraries(kagura_sample_basic PRIVATE
    "${KAGURA_DIR}/build/runtime/libkagura_runtime.a")
endif()

# ── CTest integration ─────────────────────────────────────────────────────────
enable_testing()
add_test(NAME kagura_basic_runs COMMAND kagura_sample_basic)
set_tests_properties(kagura_basic_runs PROPERTIES
  PASS_REGULAR_EXPRESSION "License OK"
  TIMEOUT 10)
