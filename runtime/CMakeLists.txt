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
  # Phase 4.3 Anti-Tamper additions
  symbol_interposition.c
  # Phase 4.3.10 JNI hook detection (Android only — compiled conditionally)
  jni_hook_detection.c
  # Phase 4.3.11 Direct syscall bypass
  direct_syscall.c
  # Phase 4.4.12 /proc inspection hardening (Linux/Android only)
  proc_inspection.c
  # Phase 4.4.6/7/9 iOS platform
  ios_platform.c
  # Phase 4.2.13 Checksum-guarded decryption
  blob_integrity.c
  # Phase 4.4.16 seccomp/prctl checks (Linux/Android)
  seccomp_checks.c
  # Phase 4.4.3 fishhook countermeasure (Apple)
  fishhook_countermeasure.c
  # Phase 4.4.5 TestFlight vs App Store detection (iOS)
  testflight_detect.c
)

target_include_directories(kagura_runtime PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(kagura_runtime PROPERTIES
  C_STANDARD 11
  POSITION_INDEPENDENT_CODE ON
)
