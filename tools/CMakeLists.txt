# kagura-opt: standalone bitcode processing tool (4.1.5 Bitcode input support)
#
# Wraps LLVM's opt infrastructure to make it easy to run kagura passes on
# .bc / .ll files:
#
#   kagura-opt -kagura-fla -kagura-sub input.bc -o output.bc
#   kagura-opt -kagura-str -kagura-anti-debug input.ll -o output.ll -S
#
# Build note
# ----------
# KaguraObfuscator is a MODULE_LIBRARY (loaded via -fpass-plugin at runtime)
# and cannot be linked into another executable target.  kagura-opt instead
# links against:
#   - KaguraObfuscatorObjects  (OBJECT library with all pass implementations)
#   - Plugin.cpp compiled directly into this target
# This gives kagura-opt access to getKaguraPluginInfo() and all passes without
# going through dlopen, so all -kagura-* cl::opt flags are registered at
# static-initialiser time just as they are in the plugin.

add_executable(kagura-opt
  kagura-opt.cpp
  # Plugin entry point compiled directly into the executable so that
  # getKaguraPluginInfo() is available for the forward declaration in
  # kagura-opt.cpp.  All other pass implementations come from the OBJECT lib.
  ${CMAKE_SOURCE_DIR}/lib/Transforms/Plugin.cpp
)

target_link_libraries(kagura-opt PRIVATE
  KaguraObfuscatorObjects
  LLVMCore
  LLVMSupport
  LLVMIRReader
  LLVMBitWriter
  LLVMBitReader
  LLVMPasses
)

target_include_directories(kagura-opt PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/lib/Transforms
  ${LLVM_INCLUDE_DIRS}
)

# Place alongside the plugin so scripts can find both in one dir.
set_target_properties(kagura-opt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS kagura-opt DESTINATION bin)
