# kagura-opt: standalone bitcode processing tool
#
# Loads libKaguraObfuscator at runtime via PassPlugin::Load() — no static link
# against the MODULE.  This avoids the MODULE_LIBRARY linkage restriction.
#
# We use add_llvm_executable (from LLVM's AddLLVM.cmake) instead of plain
# add_executable so that llvm_update_compile_flags() is applied.  Without it,
# the LLVM 17 libc++ headers on macOS are not found before the macOS SDK C
# headers, causing cascade errors from <cstddef>, <cstdint>, etc.

add_llvm_executable(kagura-opt
  kagura-opt.cpp
)

llvm_map_components_to_libnames(KAGURA_OPT_LLVM_LIBS
  Core
  Support
  IRReader
  BitWriter
  BitReader
  Passes
)

target_link_libraries(kagura-opt PRIVATE
  ${KAGURA_OPT_LLVM_LIBS}
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

# kagura-opt needs the plugin to be built first (loaded at runtime).
add_dependencies(kagura-opt KaguraObfuscator)

install(TARGETS kagura-opt DESTINATION bin)
