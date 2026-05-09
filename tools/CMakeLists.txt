# kagura-opt: standalone bitcode processing tool (4.1.5 Bitcode input support)
#
# Wraps LLVM's opt infrastructure to make it easy to run kagura passes on
# .bc / .ll files:
#
#   kagura-opt -kagura-fla -kagura-sub input.bc -o output.bc
#   kagura-opt -kagura-str -kagura-anti-debug input.ll -o output.ll -S
#
# Under the hood this is equivalent to:
#   opt -load-pass-plugin=libKaguraObfuscator.so \
#       -passes='kagura-fla,kagura-sub' input.bc -o output.bc
# but with kagura flags forwarded directly.

add_executable(kagura-opt
  kagura-opt.cpp
)

target_link_libraries(kagura-opt PRIVATE
  KaguraObfuscator
  LLVMCore
  LLVMSupport
  LLVMIRReader
  LLVMBitWriter
  LLVMBitReader
  LLVMPasses
)

target_include_directories(kagura-opt PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)

# Place alongside the plugin so scripts can find both in one dir.
set_target_properties(kagura-opt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS kagura-opt DESTINATION bin)
