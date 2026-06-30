# Configuration

Kagura accepts a JSON policy file via `-kagura-config=<path>` to control every
pass setting in one place. This is the recommended way to drive obfuscation in
real projects.

## JSON DSL

```json
{
  "profile": "BALANCED",
  "passes": {
    "str":   true,
    "fla":   true,
    "bcf":   true,
    "honey": true,
    "mvo":   false
  },
  "tuning": {
    "bcf_prob": 40,
    "seed":     12345
  }
}
```

The policy file is consumed by the **`kagura-config`** pass at the very start
of the pipeline; it sets defaults for every subsequent pass. Per-function
[`__attribute__((annotate("kagura_*")))`](getting-started/quick-start.md#5-per-function-control)
overrides still win on a function-by-function basis.

## Strength profiles

Built-in profiles selected by the `"profile"` key:

| Profile | Passes | Intended use |
|:--------|:-------|:-------------|
| `FAST`     | STR only | Debug / CI builds with minimal overhead |
| `BALANCED` | STR + BCF + BBR + BBS + GENC + MVO | Standard release builds |
| `STRONG`   | All passes, BCF prob 60, 2 iterations | Security-critical shipping builds |

A profile sets defaults; anything in `"passes"` or `"tuning"` overrides the
profile's choices for that specific key.

## Worked example — bank / FinTech release

Strong profile with per-build AES key rotation so a key extracted from one
version is useless against the next:

```json title="kagura-bank.json"
{
  "profile": "STRONG",
  "passes": {
    "str-aes":  true,
    "mvo":      true,
    "pe":       true,
    "bbcheck":  true,
    "tamper":   true
  },
  "tuning": {
    "bcf_prob": 60,
    "seed":     0
  }
}
```

```bash
clang -fpass-plugin=KaguraObfuscator.dylib \
      -mllvm -kagura-config=kagura-bank.json \
      -mllvm -kagura-build-id=$(git rev-parse HEAD) \
      -O2 -c bank_core.c -o bank_core.o
```

## See also

- [Tuning Parameters](tuning.md) — every CLI flag, including symbol filters
  and the `-kagura-build-id` per-build key seed.
- [Pass Order](pass-order.md) — the deterministic order in which the plugin
  applies these passes.
- [Game Protection](game-protection.md) — `Protected<T>` for run-time value
  protection (complementary to `mvo` / `pe`).
