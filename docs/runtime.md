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

## Platform attestation API

Thin C bindings for the major platform attestation services. The C side
generates nonces and runs fast local pre-screens; the async signed-token
round-trip is wired up from your Swift / Kotlin code.

### Apple — DeviceCheck / App Attest (`runtime/ios/device_attest.c`)

```c
int kagura_devicecheck_available(void);     // iOS 11+, macOS 10.15+
int kagura_appattest_available(void);       // iOS 14+, A10+ hardware

int kagura_appattest_nonce(uint8_t *out, size_t len);
int kagura_appattest_local_check(void);     // fast (<5ms) env screen
```

Swift bridge example:

```swift
import DeviceCheck
let service = DCAppAttestService.shared
if service.isSupported && kagura_appattest_local_check() == 1 {
    var nonce = Data(count: 32)
    _ = nonce.withUnsafeMutableBytes { kagura_appattest_nonce($0.baseAddress, 32) }
    service.generateKey { keyId, err in /* server-side verification */ }
}
```

### Android — Play Integrity (`runtime/android/play_integrity.c`)

```c
void kagura_play_integrity_nonce(char *out_hex32, size_t len);
int  kagura_play_integrity_verdict_ok(const char *jwt_payload_b64url);
int  kagura_play_integrity_local_check(void);
```

The full JWT signature must be verified server-side — `verdict_ok` is a
**local fast-path**, not a security boundary. See the file header comment
for the Kotlin caller skeleton.

### Windows — ETW analysis-tool detection (`runtime/windows/etw_detection.c`)

```c
int kagura_etw_provider_present(const wchar_t *provider_guid);
int kagura_etw_analysis_tool_check(void);   // checks Cheat Engine / Procmon / etc.
```

This module ships as a **stub** by default. Build with `-DKAGURA_ETW_FULL=1`
and link `tdh.lib` to enable the real `TdhEnumerateProviders`-based
enumeration — see the file's header comment for the implementation outline.

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
