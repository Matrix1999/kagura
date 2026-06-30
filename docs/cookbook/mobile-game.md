# Mobile game / anti-cheat

Hide game-state values (HP, currency, speed, ammo) from memory editors and
respond to scripted automation. Targeted at unmodded Android phones running
GameGuardian / Cheat Engine and rooted-device farms running Frida.

## Threat model

| Asset | Adversary | Capability |
|:------|:----------|:-----------|
| HP / damage / cooldown timers | Casual cheaters with GameGuardian | Memory scan → freeze value at "max" |
| Currency / loot rolls | Account-selling farms | Modify in-process or replay-attack server endpoints |
| Anti-cheat detection logic | Reverse engineers writing bypass scripts | Decompile + write a Frida script that hooks the check |
| Server-auth tokens | Botting frameworks | Hook the request signer to mint fake actions |

## Policy file

```json title="kagura-game.json"
{
  "profile": "BALANCED",
  "passes": {
    "str":     true,
    "wstr":    true,
    "mvo":     true,
    "pe":      true,
    "fla":     true,
    "bcf":     true,
    "sub":     true,
    "anti-debug": true,
    "tamper":  true,
    "bbcheck": false,
    "honey":   true,
    "telemetry": true
  },
  "tuning": {
    "bcf_prob": 40,
    "seed":     0
  }
}
```

Why these choices:

- **BALANCED-equivalent** — games are FPS-sensitive. STRONG profile costs
  too much frame budget; reserve `kagura_vm` for non-hot functions like the
  daily-reward signer
- **`bbcheck: false`** — per-BB checksums add 2–5% to **every** function;
  not worth it for a 60fps render loop
- **`telemetry`** — emit detection events to your server so a population of
  cheaters becomes visible even if a single client crashes / responds
  softly

## Source-side: `Protected<T>` for game state

```cpp
#include "kagura/game_protect.h"

class Player {
    kagura::Protected<int>   hp_      {100};
    kagura::Protected<int>   gold_    {0};
    kagura::Protected<float> speed_   {5.5f};
    kagura::Protected<int>   ammo_    {30};

public:
    void takeDamage(int dmg) {
        hp_ -= dmg;
        if (hp_ <= 0) die();
    }
    void rewardKill(int g) { gold_ += g; }
};

// One-time setup in main()
void initAntiCheat() {
    kagura::Protected<int>::setTamperCallback([] {
        // Soft response: don't crash — report to server, then desync the
        // player's session so future actions are rejected anyway.
        kagura_telemetry_report("tamper_detected", 1);
        // Optionally roll dice on a delayed crash to avoid giving an
        // attacker a clean detection point.
    });
}
```

Why `Protected<T>` over raw `kagura-mvo`:

| Feature | `kagura-mvo` (compile-time) | `Protected<T>` (runtime) |
|:--------|:----------------------------|:-------------------------|
| Encrypts on stack | ✅ at every store/load | ✅ |
| Detects external write | ❌ | ✅ shadow-copy mismatch |
| Per-instance key | ❌ (one PRNG seed/build) | ✅ ASLR + stack entropy |
| Crosses ABI boundaries | ❌ | ✅ |

Use **both** — `kagura-mvo` covers everything you forgot to wrap in
`Protected<T>`.

## Build

```bash
# Unity IL2CPP example — see docs/integration/unity.md for the post-build hook
clang -fpass-plugin=KaguraObfuscator.dylib \
      -mllvm -kagura-config=kagura-game.json \
      -mllvm -kagura-build-id=$(date +%Y%m%d-$BUILD_NUMBER) \
      -O2 your_il2cpp_sources.cpp -o libil2cpp.so
```

The `-kagura-build-id` ensures every release uses a different XOR mask, so a
cheat trainer built for v1.0.4 doesn't work on v1.0.5.

## Runtime checks

In your title-screen / first-frame code:

```cpp
void onFirstFrame() {
    // Probe for hook frameworks (Frida gadget, Substrate, fishhook, etc.)
    if (kagura_check_loaded_libraries() != 0) {
        // Don't kick the player — just disable competitive features
        disableLeaderboards();
        kagura_telemetry_report("hooked_libs", 1);
    }

    // Probe for debugger attach
    if (kagura_check_breakpoints() != 0 || kagura_check_emulator() != 0) {
        // Same: feature-gate, don't crash
        disableLeaderboards();
        kagura_telemetry_report("debugger_or_emu", 1);
    }
}
```

## Verification

```bash
# 1. GameGuardian-style value scan should find nothing
#    Use a debug build with Protected<int> hp(100) and run a memory scanner
#    looking for the value 100 — should not appear in a memory snapshot

# 2. Frida resistance probes
cd tests/frida_resistance
for s in probes/F*.js; do
    timeout 10 frida -l "$s" -f com.yourgame.app
done

# 3. Confirm telemetry events fire on a rooted emulator
adb -e shell setprop frida.gadget.injected 1
# launch game → expect kagura_telemetry "hooked_libs" event in your SOC
```

## What's still on you

- **Server-side authority over economy.** Any item drop / currency
  reward / leaderboard rank must be **server-authoritative**. Kagura makes
  the client harder to tamper with; it does not stop a determined attacker
  from manipulating client state. Treat the client as untrusted.
- **Replay protection on server API.** A signed request that an attacker
  intercepts can still be replayed. Use server-side nonces + timestamps.
- **Banning policy.** Telemetry without ban automation is just dashboards.
  Have a process for acting on `tamper_detected` events.
