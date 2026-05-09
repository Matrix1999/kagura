#!/usr/bin/env python3
"""
run_angr_eval.py — Symbolic execution resistance evaluation using angr.

4.8.4: Measures the analysis cost imposed on angr by kagura's obfuscation.

Usage:
    python3 run_angr_eval.py --binary /tmp/fib_obf [--timeout 30] [--entry main]

Requirements:
    pip install angr

Output (JSON to stdout):
    {
      "binary": "/tmp/fib_obf",
      "entry": "main",
      "timeout": 30,
      "paths_explored": 42,
      "time_elapsed": 12.3,
      "timed_out": false,
      "verdict": "RESISTANT"
    }

Verdict thresholds:
    RESISTANT  — timed out OR paths_explored >= 50
    PARTIAL    — 10 <= paths_explored < 50
    VULNERABLE — paths_explored < 10
"""

import argparse
import json
import sys
import time

def run_eval(binary: str, entry: str, timeout: int) -> dict:
    try:
        import angr  # type: ignore
    except ImportError:
        return {
            "error": "angr not installed — run: pip install angr",
            "binary": binary,
        }

    result = {
        "binary": binary,
        "entry": entry,
        "timeout": timeout,
        "paths_explored": 0,
        "time_elapsed": 0.0,
        "timed_out": False,
        "verdict": "UNKNOWN",
    }

    try:
        proj = angr.Project(binary, auto_load_libs=False)

        # Find entry function
        sym = proj.loader.find_symbol(entry)
        if sym is None:
            result["error"] = f"Symbol '{entry}' not found"
            return result
        entry_addr = sym.rebased_addr

        state = proj.factory.blank_state(addr=entry_addr)
        simgr = proj.factory.simulation_manager(state)

        t0 = time.time()
        deadline = t0 + timeout
        paths = 0

        while simgr.active and time.time() < deadline:
            simgr.step()
            paths += len(simgr.active)
            if paths > 500:  # cap to avoid OOM
                break

        elapsed = time.time() - t0
        timed_out = (time.time() >= deadline)

        result["paths_explored"] = paths
        result["time_elapsed"] = round(elapsed, 2)
        result["timed_out"] = timed_out

        if timed_out or paths >= 50:
            result["verdict"] = "RESISTANT"
        elif paths >= 10:
            result["verdict"] = "PARTIAL"
        else:
            result["verdict"] = "VULNERABLE"

    except Exception as e:
        result["error"] = str(e)

    return result


def main():
    parser = argparse.ArgumentParser(description="angr symbolic execution evaluator")
    parser.add_argument("--binary", required=True, help="Path to binary under test")
    parser.add_argument("--entry", default="main", help="Function to analyse")
    parser.add_argument("--timeout", type=int, default=30, help="Analysis timeout (s)")
    args = parser.parse_args()

    result = run_eval(args.binary, args.entry, args.timeout)
    print(json.dumps(result, indent=2))

    if result.get("verdict") == "VULNERABLE":
        sys.exit(1)


if __name__ == "__main__":
    main()
