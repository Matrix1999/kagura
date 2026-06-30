# Runtime Library

Some passes require linking `libkagura_runtime.a` (built at
`build/runtime/libkagura_runtime.a`):

| Pass | Required symbols |
|:-----|:-----------------|
| StringEncryptionAES | `kagura_aes128_ctr_decrypt`, `kagura_zero_buf` |
| VMObfuscation | `kagura_vm_execute` |
| AntiDebug | `kagura_anti_debug_init`, `kagura_check_hooks`, `kagura_check_breakpoints`, `kagura_check_emulator` |
| AntiTamper | `kagura_self_check`, `kagura_tamper_detected` |
| CallIndirection | `dlsym` (system) |
| PointerAuth | `kagura_random_u64` |

```bash
clang your_file.c build/runtime/libkagura_runtime.a -o your_file
```

## Directly callable anti-tamper API

`include/kagura/runtime.h` exposes the integrity checks for callers who want
them outside the pass-injected scaffolding:

```c
#include "kagura/runtime.h"

kagura_self_check();                   // Mach-O / ELF integrity + jailbreak/root
kagura_check_loaded_libraries();       // Suspicious dylib / .so scan (Frida gadgets, etc.)
kagura_run_review_risk_check();        // App Store / Play Store pre-submission scan
```

Use these from your `main()` (mobile apps) or your DLL `DllMain` (Windows) to
get the same defense without going through pass-injected init code — useful in
projects where you want explicit control over when checks fire.

## Source layout

```
runtime/
├── core/         AES, VM interpreter, crash symbolication, device key
├── anti_debug/   Cross-platform POSIX anti-debug / anti-Frida
├── android/      Root detection, attestation, /proc, syscall probes (Android + Linux)
├── ios/          Jailbreak detection, Mach-O integrity (iOS + macOS)
├── windows/      IsDebuggerPresent, NtQueryInformationProcess, PE integrity
└── game/         Anti-cheat, IL2CPP protection, telemetry
```
