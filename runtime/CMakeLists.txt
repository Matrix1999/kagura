add_library(kagura_runtime STATIC

  # ---- Core: crypto, VM, crash symbolication, device key ----------------------
  core/aes.c
  core/zero_buf.c
  core/device_key.c
  core/blob_integrity.c
  core/vm_interpreter.c
  core/crash_symbolication.c

  # ---- Anti-debug / anti-analysis (cross-platform) ---------------------------
  anti_debug/anti_debug.c
  anti_debug/breakpoint_detection.c
  anti_debug/hook_detection.c
  anti_debug/emulator_detection.c
  anti_debug/loaded_library_scan.c
  anti_debug/symbol_interposition.c
  anti_debug/anti_dump.c
  anti_debug/soft_response.c

  # ---- Android + Linux: integrity, attestation, syscall / proc hardening ------
  android/android_root_advanced.c
  android/jni_hook_detection.c
  android/play_integrity.c
  android/safetynet_compat.c
  android/art_environment.c
  android/load_order.c
  android/apk_integrity.c
  android/elf_integrity.c
  android/split_apk.c
  android/direct_syscall.c
  android/proc_inspection.c
  android/seccomp_checks.c

  # ---- iOS / macOS: integrity, jailbreak detection ----------------------------
  ios/jailbreak_detection.c
  ios/ios_integrity.c
  ios/ios_jailbreak_advanced.c
  ios/ios_platform.c
  ios/macho_integrity.c
  ios/fishhook_countermeasure.c
  ios/testflight_detect.c
  ios/swift_protection.c
  ios/objc_name_remap.c

  # ---- Game / anti-cheat ------------------------------------------------------
  game/il2cpp_protection.c
  game/ue4_protection.c
  game/game_values.c
  game/state_integrity.c
  game/behavior_log.c
  game/integrity_report.c
)

target_include_directories(kagura_runtime PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(kagura_runtime PROPERTIES
  C_STANDARD 11
  POSITION_INDEPENDENT_CODE ON
)
