# Testing & Evaluation

## Unit / regression tests

```bash
cd build && ctest --output-on-failure
```

Lit-based FileCheck tests live under `tests/`. See
`tests/regression/README.md` and `tests/pass-inputs/README.md` for the test
matrix.

## Reproducible build verification

Verify that a fixed seed produces byte-identical IR across two builds:

```bash
./scripts/verify-reproducible.sh
# [kagura-repro] PASS: Both builds produced identical IR.
```

## Differential testing

Verify that obfuscated binaries produce the same output as plain binaries:

```bash
./scripts/differential-test.sh
# [diff-test] arithmetic_test ... PASS
# [diff-test] combined_test   ... PASS
# Results: 8 passed, 0 failed, 0 skipped
```

## App Store / Google Play review risk assessment

Scan a compiled binary for patterns that may trigger store-review rejection
(non-PIE, plaintext API keys, debug symbols leaking selectors, …):

```bash
./scripts/review-risk-assessment.sh path/to/MyApp.dylib --platform ios
# [HIGH    ] [SEC-PIE] ...
# [INFO    ] [ENC-DECL] No obvious encryption keyword references found.
# RESULT: No critical or high review risks detected.
```

## Security evaluation harnesses

```bash
# Symbolic execution resistance (angr)
cd tests/symbolic_exec && python3 run_angr_eval.py \
    --binary /tmp/my_binary --timeout 30

# Decompiler resistance (Ghidra)
cd tests/decompiler_eval && python3 run_ghidra_eval.py \
    --binary /tmp/my_binary --ghidra /path/to/ghidra

# Frida instrumentation resistance (probes F1-F8)
cd tests/frida_resistance && for s in probes/F*.js; do
    frida -l "$s" -f /tmp/my_binary
done

# Full red-team report
cd tests/redteam && python3 run_redteam.py \
    --binary /tmp/my_binary --report report.json
```

Each subdirectory has its own README — see
[`tests/symbolic_exec/README.md`](https://github.com/ykus4/kagura/tree/main/tests/symbolic_exec),
[`tests/decompiler_eval/README.md`](https://github.com/ykus4/kagura/tree/main/tests/decompiler_eval),
[`tests/frida_resistance/README.md`](https://github.com/ykus4/kagura/tree/main/tests/frida_resistance),
[`tests/redteam/README.md`](https://github.com/ykus4/kagura/tree/main/tests/redteam).

## Additional analysis tools

| Script | Purpose |
|:-------|:--------|
| `scripts/kagura-cli.py` | Config generator, audit log viewer, symbol map analyzer |
| `scripts/variant_generator.py` | Per-customer / per-app variant generation with custom keys |
| `scripts/attacker_cost_model.py` | Estimate attacker reverse-engineering cost (analyst-hours) |
| `scripts/battery_impact.py` | Model battery / CPU impact of runtime passes |
| `scripts/license_manager.py` | Generate, validate, and revoke time-limited license tokens |
