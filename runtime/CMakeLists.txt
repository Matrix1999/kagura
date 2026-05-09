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
  # Phase 4.4.15 ART/JIT environment (Android)
  art_environment.c
  # Phase 4.4.17 Native library load order (Android)
  load_order.c
  # Phase 4.5.5 State invariant checks (cross-platform)
  state_integrity.c
  # Phase 4.5.7 Suspicious behavior logging (cross-platform)
  behavior_log.c
  # Phase 4.5.12/13 Game value protection (speed/seed)
  game_values.c
  # Phase 4.5.8 Soft/delayed response
  soft_response.c
  # Phase 4.2.6 Device-bound key derivation
  device_key.c
  # Phase 4.4.13 Play Integrity API integration (Android)
  play_integrity.c
  # Phase 4.4.14 SafetyNet backward compatibility (Android)
  safetynet_compat.c
  # Phase 4.6.6 Crash symbolication support (dSYM / tombstone)
  crash_symbolication.c
  # Phase 4.3.15 Anti-dump / anti-memory-scan
  anti_dump.c
  # Phase 4.5.9 + 4.5.10 Integrity report signing + nonce/challenge-response
  integrity_report.c
  # Phase 4.4.18 Split APK / AAB support (Android)
  split_apk.c
  # Phase 4.4.4 Swift metadata / demangling countermeasure (Apple)
  swift_protection.c
)

target_include_directories(kagura_runtime PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(kagura_runtime PROPERTIES
  C_STANDARD 11
  POSITION_INDEPENDENT_CODE ON
)
