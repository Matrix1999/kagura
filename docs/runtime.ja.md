# ランタイムライブラリ

一部のパスは `libkagura_runtime.a` (`build/runtime/libkagura_runtime.a` でビルド) のリンクが必要です:

| パス | 必要シンボル |
|:-----|:-------------|
| StringEncryptionAES | `kagura_aes128_ctr_decrypt`, `kagura_zero_buf` |
| VMObfuscation | `kagura_vm_execute` |
| AntiDebug | `kagura_anti_debug_init`, `kagura_check_hooks`, `kagura_check_breakpoints`, `kagura_check_emulator` |
| AntiTamper | `kagura_self_check`, `kagura_tamper_detected` |
| CallIndirection | `dlsym` (システム) |
| PointerAuth | `kagura_random_u64` |

```bash
clang your_file.c build/runtime/libkagura_runtime.a -o your_file
```

## 直接呼び出し可能なアンチタンパー API

`include/kagura/runtime.h` はパス注入スキャフォールディングの外でも呼び出せる整合性チェックを公開しています:

```c
#include "kagura/runtime.h"

kagura_self_check();                   // Mach-O / ELF 整合性 + jailbreak/root
kagura_check_loaded_libraries();       // 怪しい dylib / .so スキャン (Frida gadget 等)
kagura_run_review_risk_check();        // App Store / Play Store 提出前スキャン
```

モバイルアプリの `main()` や Windows の `DllMain` から呼び出すと、パス注入の初期化コードを経由せずに同じ防御が得られます — チェックの発火タイミングを明示的に制御したい場合に便利です。

## プラットフォーム認証 API

主要なプラットフォーム認証サービスへの薄い C バインディング。C 側はノンス生成と高速なローカル事前スクリーンを実行し、非同期署名トークンのラウンドトリップは Swift / Kotlin から接続します。

### Apple — DeviceCheck / App Attest (`runtime/ios/device_attest.c`)

```c
int kagura_devicecheck_available(void);     // iOS 11+, macOS 10.15+
int kagura_appattest_available(void);       // iOS 14+, A10+ ハードウェア

int kagura_appattest_nonce(uint8_t *out, size_t len);
int kagura_appattest_local_check(void);     // 高速 (<5ms) 環境スクリーン
```

Swift ブリッジ例:

```swift
import DeviceCheck
let service = DCAppAttestService.shared
if service.isSupported && kagura_appattest_local_check() == 1 {
    var nonce = Data(count: 32)
    _ = nonce.withUnsafeMutableBytes { kagura_appattest_nonce($0.baseAddress, 32) }
    service.generateKey { keyId, err in /* サーバー側検証 */ }
}
```

### Android — Play Integrity (`runtime/android/play_integrity.c`)

```c
void kagura_play_integrity_nonce(char *out_hex32, size_t len);
int  kagura_play_integrity_verdict_ok(const char *jwt_payload_b64url);
int  kagura_play_integrity_local_check(void);
```

JWT 署名の完全な検証はサーバー側で行う必要があります — `verdict_ok` は**ローカルの高速パス**であり、セキュリティ境界ではありません。Kotlin 呼び出しのスケルトンはファイルヘッダーコメントを参照してください。

### Windows — ETW 解析ツール検出 (`runtime/windows/etw_detection.c`)

```c
int kagura_etw_provider_present(const wchar_t *provider_guid);
int kagura_etw_analysis_tool_check(void);   // Cheat Engine / Procmon 等を検出
```

このモジュールはデフォルトで**スタブ**として出荷されます。`-DKAGURA_ETW_FULL=1` でビルドし `tdh.lib` をリンクすると、実際の `TdhEnumerateProviders` ベースの列挙を有効化できます — 実装の概要はファイルのヘッダーコメントを参照。

## ソースレイアウト

```
runtime/
├── core/         AES, VM インタプリタ, クラッシュシンボル化, デバイス鍵
├── anti_debug/   クロスプラットフォーム POSIX アンチデバッグ / アンチ Frida
├── android/      ルート検出, アテステーション, /proc, syscall プローブ (Android + Linux)
├── ios/          Jailbreak 検出, Mach-O 整合性 (iOS + macOS)
├── windows/      IsDebuggerPresent, NtQueryInformationProcess, PE 整合性
└── game/         アンチチート, IL2CPP 保護, テレメトリ
```
