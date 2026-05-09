add_library(kagura_runtime STATIC

  # ---- Core crypto / shared utilities ----------------------------------------
  aes.c
  zero_buf.c
  device_key.c
  blob_integrity.c

  # ---- Anti-debug / anti-analysis (cross-platform) ---------------------------
  anti_debug.c
  breakpoint_detection.c
  hook_detection.c
  emulator_detection.c
  loaded_library_scan.c
  symbol_interposition.c
  anti_dump.c
  soft_response.c

  # ---- Linux / Android — syscall / proc hardening ----------------------------
  direct_syscall.c
  proc_inspection.c
  seccomp_checks.c

  # ---- Android integrity & attestation ---------------------------------------
  android_root_advanced.c
  jni_hook_detection.c
  play_integrity.c
  safetynet_compat.c
  art_environment.c
  load_order.c
  apk_integrity.c
  elf_integrity.c
  split_apk.c

  # ---- iOS / macOS integrity & jailbreak detection ---------------------------
  jailbreak_detection.c
  ios_integrity.c
  ios_jailbreak_advanced.c
  ios_platform.c
  macho_integrity.c
  fishhook_countermeasure.c
  testflight_detect.c
  swift_protection.c
  objc_name_remap.c

  # ---- Anti-cheat / game integrity -------------------------------------------
  il2cpp_protection.c
  game_values.c
  state_integrity.c
  behavior_log.c
  integrity_report.c

  # ---- Crash symbolication ---------------------------------------------------
  crash_symbolication.c

  # ---- VM interpreter --------------------------------------------------------
  vm_interpreter.c
)

target_include_directories(kagura_runtime PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(kagura_runtime PROPERTIES
  C_STANDARD 11
  POSITION_INDEPENDENT_CODE ON
)
