# Symbolic Execution Resistance Evaluation (4.8.4)

Evaluates kagura's effectiveness against symbolic execution tools
(angr, Triton, KLEE) by measuring analysis coverage on obfuscated vs.
plain functions.

## Methodology

For each test subject and each kagura pass combination:

1. Compile the subject to binary (with and without kagura).
2. Run angr / Triton symbolic execution with a fixed timeout.
3. Record: paths explored, time to first solution, memory usage.
4. Report: pass combination → analysis cost multiplier.

**High cost multiplier = better resistance.**

## Quick start

```bash
pip install angr
python3 run_angr_eval.py --binary /tmp/kagura_fib_fla --timeout 30
```

## Evaluation subjects

- `subjects/fibonacci` — iterative fibonacci (simple CFG)
- `subjects/checksum`  — FNV-1a hash loop (data-dependent branching)
- `subjects/dispatch`  — function pointer dispatch table (IBR target)

## Expected results

| Pass combination  | angr paths | Time (s) | vs. baseline |
|:------------------|:----------:|:--------:|:------------:|
| Baseline (none)   | 12         | 0.8s     | 1.0x         |
| FLA               | 80+        | >30s     | 37x+         |
| FLA + BCF         | 200+       | timeout  | >>100x       |
| FLA + BCF + SUB   | 200+       | timeout  | >>100x       |

(Results vary by angr version and hardware; the point is directional.)
