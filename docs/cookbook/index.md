# Cookbook

Practical, copy-pasteable recipes for common deployment scenarios.

| Scenario | What you'll get |
|:---------|:----------------|
| [**Banking / FinTech**](banking.md) | Protect HMAC keys, certificate pins, transaction-signing logic |
| [**Mobile game / anti-cheat**](mobile-game.md) | Hide HP / currency / speed from memory editors; respond to Frida |
| [**SDK / library vendor**](sdk-vendor.md) | Ship a `.a` / `.so` consumers can link without changing their build |
| [**DRM / license enforcement**](drm.md) | Virtualize license-check functions so they're never readable as native code |

Each recipe shows:

1. The threat model for that scenario
2. A complete `kagura.json` policy
3. Build commands
4. Verification steps you can run yourself

If you're not sure which one fits, start with the
[Security Model](../security-model.md) page to figure out your threat model
first.
