# 4.8.7: Regression corpus — run each known-crash .ll file through the
# relevant pass and verify it does NOT crash (exit 0).
#
# Each corpus entry is named crash_NNNNN.ll and has a companion
# crash_NNNNN.meta file with:
#   PASS=<pass-name>       (e.g. "function(kagura-fla)")
#   DESCRIPTION=<text>

cmake_minimum_required(VERSION 3.20)

set(CORPUS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/corpus)
set(KAGURA_PLUGIN $<TARGET_FILE:KaguraObfuscator>)
set(LLVM_BIN ${LLVM_TOOLS_BINARY_DIR})

# Auto-discover all .ll files in corpus/
file(GLOB CORPUS_FILES "${CORPUS_DIR}/*.ll")

foreach(LL_FILE ${CORPUS_FILES})
  get_filename_component(BASE ${LL_FILE} NAME_WE)
  set(META_FILE "${CORPUS_DIR}/${BASE}.meta")

  # Default pass if no meta file
  set(PASS_PIPELINE "function(kagura-fla,kagura-sub,kagura-bcf)")
  if(EXISTS ${META_FILE})
    file(STRINGS ${META_FILE} META_LINES)
    foreach(LINE ${META_LINES})
      if(LINE MATCHES "^PASS=(.+)$")
        set(PASS_PIPELINE "${CMAKE_MATCH_1}")
      endif()
    endforeach()
  endif()

  add_test(NAME regression_${BASE}
    COMMAND ${LLVM_BIN}/opt
      --load-pass-plugin=${KAGURA_PLUGIN}
      -passes=${PASS_PIPELINE}
      ${LL_FILE}
      -o /dev/null
      -disable-output
  )
  set_tests_properties(regression_${BASE} PROPERTIES
    DEPENDS KaguraObfuscator
    TIMEOUT 30
    LABELS "regression"
  )
endforeach()
