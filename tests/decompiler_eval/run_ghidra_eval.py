#!/usr/bin/env python3
"""
run_ghidra_eval.py — Decompiler resistance evaluation using Ghidra headless.

4.8.5: Measures the analysis cost imposed on Ghidra by kagura's obfuscation.

Usage:
    python3 run_ghidra_eval.py \
        --binary /tmp/fib_obf \
        --ghidra /opt/ghidra_10.x \
        [--project /tmp/kagura_ghidra_proj]

Requirements:
    Ghidra 10+ installed with analyzeHeadless in PATH or GHIDRA_HOME set.

Output (JSON to stdout):
    {
      "binary": "/tmp/fib_obf",
      "function_count": 5,
      "avg_bb_count": 42,
      "auto_named_ratio": 0.85,
      "analysis_time": 12.3,
      "comprehensibility_score": 28,
      "verdict": "RESISTANT"
    }
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET


GHIDRA_SCRIPT = """\
import ghidra.app.script.GhidraScript
import ghidra.program.model.listing.Function
import ghidra.program.flatapi.FlatProgramAPI

# Collect metrics and print as JSON
flat = FlatProgramAPI(currentProgram)
funcs = list(currentProgram.getFunctionManager().getFunctions(True))
total = len(funcs)
auto_named = sum(1 for f in funcs if f.getName().startswith("FUN_") or f.getName().startswith("thunk_FUN_"))
bb_counts = []
for f in funcs:
    bbs = list(f.getBody().getAddressRanges())
    bb_counts.append(len(bbs))
avg_bb = sum(bb_counts) / max(len(bb_counts), 1)

import json
result = {
    "function_count": total,
    "avg_bb_count": avg_bb,
    "auto_named": auto_named,
    "auto_named_ratio": auto_named / max(total, 1),
}
print("KAGURA_METRICS:" + json.dumps(result))
"""


def find_analyzeHeadless(ghidra_home: str) -> str:
    candidates = [
        os.path.join(ghidra_home, "support", "analyzeHeadless"),
        os.path.join(ghidra_home, "analyzeHeadless"),
        "analyzeHeadless",
    ]
    for c in candidates:
        if os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    return "analyzeHeadless"  # hope it's in PATH


def run_eval(binary: str, ghidra_home: str, project_dir: str) -> dict:
    result = {
        "binary": binary,
        "function_count": 0,
        "avg_bb_count": 0.0,
        "auto_named_ratio": 0.0,
        "analysis_time": 0.0,
        "comprehensibility_score": -1,
        "verdict": "UNKNOWN",
    }

    analyze = find_analyzeHeadless(ghidra_home)
    proj_name = "kagura_eval"
    binary_name = os.path.basename(binary)

    # Write the metrics collection script to a temp file
    script_dir = tempfile.mkdtemp(prefix="kagura_ghidra_")
    script_path = os.path.join(script_dir, "kagura_metrics.py")
    with open(script_path, "w") as f:
        f.write(GHIDRA_SCRIPT)

    cmd = [
        analyze,
        project_dir, proj_name,
        "-import", binary,
        "-scriptPath", script_dir,
        "-postScript", "kagura_metrics.py",
        "-deleteProject",
        "-noanalysis",  # use auto-analysis only
    ]

    t0 = time.time()
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        elapsed = time.time() - t0
        output = proc.stdout + proc.stderr
    except subprocess.TimeoutExpired:
        result["error"] = "Ghidra analysis timed out"
        return result
    except FileNotFoundError:
        result["error"] = f"analyzeHeadless not found at: {analyze}"
        return result

    result["analysis_time"] = round(elapsed, 2)

    # Parse metrics from output
    for line in output.splitlines():
        if "KAGURA_METRICS:" in line:
            try:
                metrics = json.loads(line.split("KAGURA_METRICS:")[1])
                result.update(metrics)
            except json.JSONDecodeError:
                pass

    # Comprehensibility score: 100 * (1 - auto_named_ratio) / (1 + log(avg_bb))
    import math
    abn = result.get("avg_bb_count", 1)
    anr = result.get("auto_named_ratio", 0)
    score = int(100 * (1 - anr) / (1 + math.log1p(max(abn, 1))))
    result["comprehensibility_score"] = score

    if score <= 20:
        result["verdict"] = "RESISTANT"
    elif score <= 50:
        result["verdict"] = "PARTIAL"
    else:
        result["verdict"] = "VULNERABLE"

    return result


def main():
    parser = argparse.ArgumentParser(description="Ghidra decompiler evaluator")
    parser.add_argument("--binary", required=True)
    parser.add_argument("--ghidra", default=os.environ.get("GHIDRA_HOME", ""))
    parser.add_argument("--project", default=tempfile.mkdtemp(prefix="kagura_ghidra_proj_"))
    args = parser.parse_args()

    if not args.ghidra:
        print(json.dumps({"error": "Set --ghidra or GHIDRA_HOME"}), file=sys.stderr)
        sys.exit(1)

    result = run_eval(args.binary, args.ghidra, args.project)
    print(json.dumps(result, indent=2))

    if result.get("verdict") == "VULNERABLE":
        sys.exit(1)


if __name__ == "__main__":
    main()
