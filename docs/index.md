# Kagura

> **LLVM-based code obfuscation and anti-tamper toolkit for mobile, desktop, and WebAssembly targets.**

Built on the **New Pass Manager** (LLVM 17+). Loaded as a pass plugin via `-fpass-plugin` — no LLVM source tree modification required.

**Supported platforms:** iOS · Android · macOS · Windows (MSVC/Clang-CL) · Linux · WebAssembly

---

## Why Kagura?

Shipping compiled native code means shipping a reverse-engineer's starting point. Static analysis tools (IDA Pro, Ghidra, Binary Ninja) and dynamic instrumentation frameworks (Frida, Substrate) can reconstruct logic, extract keys, and bypass security checks within hours on unprotected binaries.

Kagura addresses this at the IR level — before the compiler turns IR into machine code — so every protection is architecture-agnostic and works across all targets from a single build step.

| Threat | Kagura countermeasure |
|:-------|:----------------------|
| Static string extraction (`strings`, IDA imports) | `kagura-str` / `kagura-str-aes` — strings are XOR/AES-encrypted blobs until first use |
| Decompiler-readable control flow | `kagura-fla` + `kagura-bcf` — CFG becomes a switch-dispatched state machine with opaque dead branches |
| Memory editor / GameGuardian value freeze | `kagura-mvo` / `kagura-pe` / `Protected<T>` — values stored XOR-encrypted at every alloca site |
| Frida / Substrate dynamic instrumentation | `kagura-anti-debug` + loaded-library scan — detects and responds to hooking frameworks at runtime |
| Binary patching (NOP-ing integrity checks) | `kagura-bbcheck` — per-basic-block opcode checksums abort on binary modification |
| Import table analysis (IDA external calls) | `kagura-ci` — external calls routed through runtime-resolved thunk table |
| Jailbreak / root detection bypass | Runtime module: Mach-O integrity, ELF tampering, Magisk/Zygisk/LSPosed detection |

---

## Documentation map

<div class="grid cards" markdown>

- :material-rocket-launch: **[Getting Started](getting-started/quick-start.md)**

    Install, build, and run your first obfuscated binary in under five minutes.

- :material-puzzle: **[Passes](passes/index.md)**

    Reference for every IR pass — flags, effects, code-size and runtime overhead.

- :material-cog: **[Configuration](configuration.md)**

    JSON policy DSL, strength profiles, tuning parameters, per-function annotations.

- :material-shield-lock: **[Game Protection](game-protection.md)**

    `Protected<T>` template for HP, currency, and other anti-cheat-sensitive values.

- :material-toolbox: **[Integration](integration/index.md)**

    Xcode, Gradle/NDK, Unity, Unreal, CMake, Bazel, CocoaPods, SPM.

- :material-test-tube: **[Testing & Evaluation](testing.md)**

    Differential testing, reproducible builds, angr / Ghidra / Frida resistance.

- :material-server: **[Runtime Library](runtime.md)**

    Symbols required by each pass, anti-tamper API, callable checks.

- :material-format-list-numbered: **[Pass Order](pass-order.md)**

    The deterministic pipeline the plugin registers with `OptimizerLast`.

</div>

---

## At a glance

```bash
clang -fpass-plugin=KaguraObfuscator.dylib \
      -mllvm -kagura-config=kagura.json \
      -O1 your_file.c -o your_file
```

```json
{
  "profile": "BALANCED",
  "passes": { "str": true, "fla": true, "bcf": true, "mvo": true },
  "tuning": { "bcf_prob": 40, "seed": 12345 }
}
```

See **[Quick Start](getting-started/quick-start.md)** for a full walkthrough.

---

## Citation

If you use Kagura in your research or build on it, please cite the [paper](https://zenodo.org/records/20361447):

```bibtex
@software{kagura,
  author    = {yotti},
  title     = {Kagura: LLVM-based Code Obfuscation and Anti-Tamper Toolkit},
  year      = {2025},
  publisher = {Zenodo},
  doi       = {10.5281/zenodo.20361447},
  url       = {https://doi.org/10.5281/zenodo.20361447}
}
```

## License

MIT — see [LICENSE](https://github.com/ykus4/kagura/blob/main/LICENSE).
