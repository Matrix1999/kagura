# Decompiler Resistance Evaluation (4.8.5)

Evaluates kagura's effectiveness against decompilers (Ghidra, IDA Pro,
Binary Ninja) by measuring the quality of decompiled output on obfuscated
vs. plain binaries.

## Methodology

For each test subject and each kagura pass combination:

1. Compile the subject to binary (with and without kagura).
2. Run the decompiler's headless analysis API.
3. Score the output using heuristics:
   - Variable name recovery rate (auto-named vars signal failed analysis)
   - Function boundary detection accuracy
   - Control flow graph node count (more nodes = harder analysis)
   - Decompilation time (longer = harder)
4. Report: pass combination → comprehensibility score (lower = more resistant).

## Tools

### Ghidra (headless)

```bash
# Requires Ghidra 10+ with headless analyzeHeadless script
export GHIDRA_HOME=/opt/ghidra_10.x

python3 run_ghidra_eval.py \
  --binary /tmp/kagura_fib_fla \
  --ghidra $GHIDRA_HOME
```

### Binary Ninja (API)

```bash
# Requires Binary Ninja with Python API
python3 run_binja_eval.py --binary /tmp/kagura_fib_fla
```

## Expected results (directional)

| Pass             | Ghidra CFG nodes | Auto-named vars | Score |
|:-----------------|:----------------:|:---------------:|:-----:|
| Baseline (none)  | 12               | 0%              | 100   |
| FLA              | 50+              | 60%+            | 30    |
| FLA + BCF + SUB  | 100+             | 90%+            | 10    |
| FLA + VM         | dispatcher only  | 100%            | 5     |

Score 100 = fully comprehensible; 0 = completely opaque.
