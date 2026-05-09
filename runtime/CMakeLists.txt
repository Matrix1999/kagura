add_library(kagura_runtime STATIC
  anti_debug.c
  aes.c
  il2cpp_protection.c
  jailbreak_detection.c
  vm_interpreter.c
  hook_detection.c
  breakpoint_detection.c
  emulator_detection.c
  zero_buf.c
  macho_integrity.c
  apk_integrity.c
  elf_integrity.c
  ios_integrity.c
  ios_jailbreak_advanced.c
  loaded_library_scan.c
  android_root_advanced.c
)

target_include_directories(kagura_runtime PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(kagura_runtime PROPERTIES
  C_STANDARD 11
  POSITION_INDEPENDENT_CODE ON
)
