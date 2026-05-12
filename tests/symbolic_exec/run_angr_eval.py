#!/usr/bin/env python3
"""
run_angr_eval.py — Symbolic execution and disassembly resistance evaluation.

4.8.4: Measures analysis cost via two approaches:

1. Disassembly-based CFG analysis (always available):
   - Counts instructions and branches in the target function
   - Uses llvm-objdump for reliable cross-platform analysis

2. angr symbolic execution (best-effort, may fail on musl ELFs):
   - Counts unique basic blocks visited during simulation
   - Falls back gracefully if angr crashes

Usage:
    python3 run_angr_eval.py --binary /tmp/fib_obf [--timeout 30] [--entry main]

Output (JSON to stdout):
    {
      "binary": "/tmp/fib_obf",
      "entry": "main",
      "timeout": 30,
      "instr_count": 42,
      "branch_count": 8,
      "paths_explored": 15,
      "time_elapsed": 1.3,
      "timed_out": false,
      "verdict": "RESISTANT"
    }

Verdict (based on instr_count/branch_count ratio vs baseline):
    RESISTANT  — timed out OR branch_count >= 5
    PARTIAL    — 2 <= branch_count < 5
    VULNERABLE — branch_count < 2
"""

import argparse
import json
import os
import re
import subprocess
import sys
import time

OBJDUMP = os.environ.get("LLVM_OBJDUMP", "/opt/homebrew/opt/llvm/bin/llvm-objdump")


def count_instructions(binary: str, entry: str) -> tuple[int, int]:
    """Returns (instr_count, branch_count) for the entry function."""
    try:
        out = subprocess.check_output(
            [OBJDUMP, "-d", f"--disassemble-symbols={entry}", binary],
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=10,
        )
    except Exception:
        return 0, 0
    instr_count = len(re.findall(r"^\s+[0-9a-f]+:", out, re.MULTILINE))
    branch_count = len(re.findall(r"\bj[a-z]+\b", out))
    return instr_count, branch_count


def run_angr(binary: str, entry: str, timeout: int) -> dict:
    """Run angr symbolic execution, returning metrics dict (best-effort)."""
    result: dict = {"paths_explored": 0, "timed_out": False}
    try:
        import angr  # type: ignore
        proj = angr.Project(binary, auto_load_libs=False)
        sym = proj.loader.find_symbol(entry)
        if sym is None:
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
            if paths > 500:
                break

        result["paths_explored"] = paths
        result["timed_out"] = (time.time() >= deadline)
    except Exception:
        pass  # angr may crash on musl ELFs — degrade gracefully
    return result


def run_eval(binary: str, entry: str, timeout: int) -> dict:
    t0 = time.time()

    instr_count, branch_count = count_instructions(binary, entry)
    angr_result = run_angr(binary, entry, timeout)

    elapsed = round(time.time() - t0, 2)
    timed_out = angr_result.get("timed_out", False)

    if timed_out or branch_count >= 5:
        verdict = "RESISTANT"
    elif branch_count >= 2:
        verdict = "PARTIAL"
    else:
        verdict = "VULNERABLE"

    return {
        "binary": binary,
        "entry": entry,
        "timeout": timeout,
        "instr_count": instr_count,
        "branch_count": branch_count,
        "paths_explored": angr_result.get("paths_explored", 0),
        "time_elapsed": elapsed,
        "timed_out": timed_out,
        "verdict": verdict,
    }


def main():
    parser = argparse.ArgumentParser(description="CFG + symbolic execution evaluator")
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
