# Kagura Roadmap

## Phase 1-3 (Completed)

Phase 1-3ではコアの難読化パス、ランタイムライブラリ、CI/CD、ゲームエンジン統合を実装済み。

### 実装済み機能

- 10種のCFG難読化パス (FLA, BCF, IBR, CI, LT, FSplit, BBS, BBR, DCI, VM)
- 5種のデータ難読化パス (STR, STR-AES, CO, SUB, GENC)
- Anti-analysis (AntiDebug, AntiTamper, PAC, SymbolVisibility)
- Platform (ObjC, JNI)
- Runtime library (AES復号, VM実行, hook/breakpoint/emulator検知)
- Integration (Xcode, Gradle, Unity IL2CPP, Unreal, CMake toolchain)
- CI (GitHub Actions, LLVM 19/21/22 matrix)
- Metrics reporting

---

## Phase 4: Production-Grade Hardening

Phase 4はkaguraを**商用レベルのプロテクションツール**に引き上げるフェーズ。
研究プロトタイプからプロダクション品質への移行を目指す。

---

### Phase 4.1 — LLVM Infrastructure Hardening

> 目標: LLVM全バージョン・全最適化レベルで安定動作

| ID | Feature | Priority | Effort |
|:---|:--------|:---------|:-------|
| 4.1.1 | LTO / ThinLTO パイプライン対応 | 高 | M |
| 4.1.2 | O0動作モード (デバッグビルド向け軽量保護) | 高 | S |
| 4.1.3 | legacy pass manager互換レイヤー (LLVM 14-16) | 低 | L |
| 4.1.4 | loop pass分離 (LoopTransformをLoopPassManager配下に) | 中 | S |
| 4.1.5 | bitcode入力対応 (opt以外のフロー) | 中 | S |
| 4.1.6 | DWARF debug情報除去/変換制御 | 高 | M |
| 4.1.7 | target triple別処理分岐 (ARM64/ARMv7/x86_64) | 高 | M |
| 4.1.8 | arm64e / PAC hardware連携 | 中 | M |
| 4.1.9 | sanitizer互換性 (ASan, TSan, UBSan) | 中 | M |
| 4.1.10 | exception handling (C++ EH, ObjC @try) 安全性 | 高 | M |
| 4.1.11 | RTTI / vtable保護 | 中 | L |
| 4.1.12 | reproducible build / deterministic output検証 | 高 | S |

**Effort**: S = ~1日, M = 2-5日, L = 1-2週

---

### Phase 4.2 — Advanced Encryption & Data Protection

> 目標: 全データ型の暗号化カバレッジ、runtime復号の堅牢化

| ID | Feature | Priority | Effort |
|:---|:--------|:---------|:-------|
| 4.2.1 | wide string / UTF-16 / CFString暗号化 | 高 | M |
| 4.2.2 | ObjC selector文字列保護 (メタデータレベル) | 高 | M |
| 4.2.3 | ObjC class名/メソッド名難読化 | 中 | L |
| 4.2.4 | lazy decryption (初回アクセス時のみ復号) | 高 | M |
| 4.2.5 | short-lived decrypted buffer (使用後ゼロクリア) | 高 | S |
| 4.2.6 | device-bound key導出 (UDID/Android ID由来) | 中 | M |
| 4.2.7 | build-time key rotation (ビルド毎に鍵変更) | 中 | S |
| 4.2.8 | per-customer / per-app変種生成 | 低 | L |
| 4.2.9 | native constant encryption (float/double含む) | 中 | M |
| 4.2.10 | encrypted lookup table (table encoding) | 低 | L |
| 4.2.11 | white-box crypto的処理 | 低 | XL |
| 4.2.12 | network endpoint / API key / config blob保護 | 高 | M |
| 4.2.13 | checksum付き復号 (tamper時の復号失敗設計) | 中 | M |

---

### Phase 4.3 — Anti-Tamper & Integrity

> 目標: バイナリ改変・動的攻撃の多層検知

| ID | Feature | Priority | Effort |
|:---|:--------|:---------|:-------|
| 4.3.1 | Mach-O構造改変検知 (LC_LOAD_DYLIB, code signature blob) | 高 | M |
| 4.3.2 | ELF構造改変検知 (section hash, PHT検証) | 高 | M |
| 4.3.3 | iOS code signing状態確認 (embedded.mobileprovision) | 高 | M |
| 4.3.4 | Android APK signature検証 (v2/v3/v4) | 高 | M |
| 4.3.5 | dynamic library injection検知 (DYLD_INSERT) | 高 | S |
| 4.3.6 | loaded module検査 (suspicious dylib/so) | 高 | M |
| 4.3.7 | GOT/PLT hook検知 | 高 | M |
| 4.3.8 | symbol interposition検知 | 中 | M |
| 4.3.9 | ObjC method swizzling検知 | 高 | M |
| 4.3.10 | JNI table hook検知 | 中 | M |
| 4.3.11 | syscall直接呼び出し (hook回避) | 中 | M |
| 4.3.12 | hardware breakpoint検知 | 中 | S |
| 4.3.13 | memory page permission検査 (W+X検知) | 中 | S |
| 4.3.14 | app repackaging検知 | 高 | M |
| 4.3.15 | anti-dump / anti-memory-scan | 低 | L |
| 4.3.16 | basic block level checksum (細粒度整合性) | 低 | L |

---

### Phase 4.4 — Platform-Specific Hardening

> 目標: iOS/Android固有の攻撃ベクトルへの対応

#### iOS

| ID | Feature | Priority | Effort |
|:---|:--------|:---------|:-------|
| 4.4.1 | jailbreak filesystem artifact検知 | 高 | M |
| 4.4.2 | Cydia/Substrate/FridaGadget.dylib検知 | 高 | S |
| 4.4.3 | fishhook対策 (rebinding検知) | 中 | M |
| 4.4.4 | Swift metadata/demangling対策 | 中 | L |
| 4.4.5 | TestFlight/production差分対応 | 低 | S |
| 4.4.6 | iOS simulator除外 (TARGET_OS_SIMULATOR) | 中 | S |
| 4.4.7 | entitlements確認 | 中 | S |
| 4.4.8 | App Store審査安全性確認 | 高 | M |
| 4.4.9 | dyld image list動的検査 | 中 | S |

#### Android

| ID | Feature | Priority | Effort |
|:---|:--------|:---------|:-------|
| 4.4.10 | Magisk/Zygisk検知 | 高 | M |
| 4.4.11 | Xposed/LSPosed検知 | 高 | M |
| 4.4.12 | /proc検査強化 (maps/status/mounts/fd) | 中 | M |
| 4.4.13 | Play Integrity API連携 | 高 | M |
| 4.4.14 | SafetyNet後方互換 | 低 | S |
| 4.4.15 | ART/JIT環境考慮 | 中 | M |
| 4.4.16 | seccomp/prctl系チェック | 低 | M |
| 4.4.17 | native library load順制御 | 中 | S |
| 4.4.18 | split APK / AAB対応 | 中 | M |
| 4.4.19 | libil2cpp.so / libUE4.so保護特化 | 中 | M |

---

### Phase 4.5 — Game / Anti-Cheat

> 目標: ゲームクライアント固有の保護機能

| ID | Feature | Priority | Effort |
|:---|:--------|:---------|:-------|
| 4.5.1 | memory value obfuscation (XOR/rotate in-memory) | 高 | M |
| 4.5.2 | pointer encryption (アドレス難読化) | 中 | M |
| 4.5.3 | honey value / decoy variable | 高 | M |
| 4.5.4 | fake function / fake symbol (attacker誘導) | 中 | S |
| 4.5.5 | state integrity check (client-side invariant) | 中 | M |
| 4.5.6 | telemetry挿入 (cheat signal収集) | 中 | M |
| 4.5.7 | suspicious behavior logging | 中 | M |
| 4.5.8 | delayed/soft response設計 (即banしない) | 低 | S |
| 4.5.9 | integrity report署名 + replay防止 | 低 | M |
| 4.5.10 | nonce/challenge-response (server連携) | 中 | M |
| 4.5.11 | damage/hit/cooldown/currency保護 (template) | 高 | M |
| 4.5.12 | speed/movement値保護 | 中 | S |
| 4.5.13 | random seed保護 | 中 | S |

---

### Phase 4.6 — Build System & Developer Experience

> 目標: 大規模プロジェクトでの実用性、運用品質

| ID | Feature | Priority | Effort |
|:---|:--------|:---------|:-------|
| 4.6.1 | config DSL (YAML/JSON policy file) | 高 | M |
| 4.6.2 | obfuscation strength profile (FAST/BALANCED/STRONG) | 高 | M |
| 4.6.3 | annotation/macroで保護対象指定 (拡張) | 高 | S |
| 4.6.4 | allowlist / denylist (symbol/file/module単位) | 高 | M |
| 4.6.5 | symbol map出力 (難読化前→後マッピング) | 高 | M |
| 4.6.6 | crash symbolication支援 (dSYM/tombstone) | 高 | L |
| 4.6.7 | incremental build対応 | 中 | L |
| 4.6.8 | build cache対応 | 中 | M |
| 4.6.9 | multi-flavor対応 (staging/production) | 中 | M |
| 4.6.10 | audit log (何をどう保護したかの記録) | 中 | S |
| 4.6.11 | Bazel対応 | 低 | M |
| 4.6.12 | CocoaPods / SwiftPM対応 | 中 | M |
| 4.6.13 | CLI tool (config生成/レポート表示) | 中 | M |
| 4.6.14 | license管理 (商用配布向け) | 低 | M |

---

### Phase 4.7 — Testing & Quality

> 目標: リグレッション防止、信頼性の証明

| ID | Feature | Priority | Effort |
|:---|:--------|:---------|:-------|
| 4.7.1 | FileCheck lit tests (per-pass IR検証) | 高 | L |
| 4.7.2 | integration tests (real app smoke) | 高 | L |
| 4.7.3 | LLVM version matrix拡張 (17, 18追加) | 中 | M |
| 4.7.4 | NDK version matrix | 中 | M |
| 4.7.5 | fuzzing (pass入力のcrash検出) | 中 | L |
| 4.7.6 | differential testing (obfuscated vs plain動作一致) | 高 | L |
| 4.7.7 | performance benchmark (binary size / startup / frame time) | 中 | M |
| 4.7.8 | battery impact測定 | 低 | M |
| 4.7.9 | device farm tests (iOS/Android実機) | 低 | XL |
| 4.7.10 | false positive評価 (正常アプリ誤検知率) | 中 | M |
| 4.7.11 | App Store/Google Play審査リスク評価 | 高 | M |

---

### Phase 4.8 — Research & Advanced

> 目標: 攻撃者コスト最大化、自動化への対抗

| ID | Feature | Priority | Effort |
|:---|:--------|:---------|:-------|
| 4.8.1 | obfuscation transform自動選択 (risk-based) | 中 | XL |
| 4.8.2 | hot path回避 (パフォーマンスクリティカルパス保護除外) | 高 | M |
| 4.8.3 | attacker cost modeling | 低 | L |
| 4.8.4 | symbolic execution耐性評価 (angr/Triton) | 中 | L |
| 4.8.5 | decompiler耐性評価 (Ghidra/IDA/Binary Ninja) | 中 | L |
| 4.8.6 | Frida script耐性評価 | 中 | M |
| 4.8.7 | regression corpus (known-attack再現テスト) | 中 | L |
| 4.8.8 | red-team用評価ツール | 低 | L |
| 4.8.9 | ML/heuristicによる保護対象自動推定 | 低 | XL |
| 4.8.10 | machine code/backend寄りobfuscation | 低 | XL |

---

## Implementation Priority (Phase 4 First Wave)

Phase 4の最初のスプリントで着手すべき高優先度項目:

```
Sprint 1 (4.1 Infrastructure):
  4.1.1  LTO/ThinLTO
  4.1.6  DWARF制御
  4.1.7  target triple別処理
  4.1.10 exception handling安全性
  4.1.12 reproducible build検証

Sprint 2 (4.2 + 4.3 Encryption & Integrity):
  4.2.1  wide string対応
  4.2.4  lazy decryption
  4.2.5  short-lived buffer
  4.2.12 endpoint/API key保護
  4.3.1  Mach-O改変検知
  4.3.4  APK signature検証
  4.3.5  dylib injection検知
  4.3.7  GOT/PLT hook検知

Sprint 3 (4.4 Platform):
  4.4.1  jailbreak検知
  4.4.2  Cydia/Substrate検知
  4.4.10 Magisk/Zygisk検知
  4.4.13 Play Integrity連携

Sprint 4 (4.5 + 4.6 Game & DX):
  4.5.1  memory value obfuscation
  4.5.3  honey value
  4.5.11 game logic保護テンプレート
  4.6.1  config DSL
  4.6.2  strength profile
  4.6.5  symbol map出力

Sprint 5 (4.7 Quality):
  4.7.1  FileCheck tests
  4.7.6  differential testing
  4.7.11 審査リスク評価
```

---

## Effort Estimates

| Phase | Items | Est. Total |
|:------|:------|:-----------|
| 4.1 Infrastructure | 12 | 3-4週 |
| 4.2 Encryption | 13 | 4-6週 |
| 4.3 Anti-Tamper | 16 | 4-6週 |
| 4.4 Platform | 19 | 5-7週 |
| 4.5 Game | 13 | 3-5週 |
| 4.6 Build/DX | 14 | 4-6週 |
| 4.7 Testing | 11 | 4-6週 |
| 4.8 Research | 10 | 6-10週 |
| **Total** | **108** | **33-50週** |

---

## Non-Goals (Phase 4)

以下はPhase 4のスコープ外:

- サーバーサイドコンポーネント (dashboard, remote config等は将来Phase 5)
- GUI/Electron app
- Webベースのobfuscation
- ソースコードレベルobfuscation (C preprocessor tricks等)
- Java/Kotlin bytecodeレベル難読化 (ProGuard/R8の領域)
