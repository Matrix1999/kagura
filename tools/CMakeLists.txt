# kagura-opt: standalone bitcode processing tool (4.1.5 Bitcode input support)
#
# Loads libKaguraObfuscator at runtime via PassPlugin::Load() — no static link
# against the MODULE.  This avoids the MODULE_LIBRARY linkage restriction and
# the header-search-path issues that arise when Plugin.cpp is compiled in a
# different CMake target.

add_executable(kagura-opt
  kagura-opt.cpp
)

target_link_libraries(kagura-opt PRIVATE
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

# Place alongside the plugin so the auto-detection logic in kagura-opt.cpp
# can find libKaguraObfuscator in <exe_dir>/../lib/.
set_target_properties(kagura-opt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# kagura-opt needs the plugin to be built first so it can load it at runtime.
add_dependencies(kagura-opt KaguraObfuscator)

install(TARGETS kagura-opt DESTINATION bin)
