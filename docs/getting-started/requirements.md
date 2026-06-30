# Requirements

| Requirement | Version |
|:------------|:--------|
| LLVM        | 17 – 22 (tested on 17, 18, 19, 21, 22) |
| CMake       | 3.20+ |
| C++         | C++17 |

## Platform notes

- **Windows** — Clang-CL only. The plugin builds as a static library
  (`KaguraObfuscator.lib`) because MSVC does not support loadable pass modules.
  Link it directly into your driver tool.

- **WebAssembly** — `kagura-fla` and `kagura-anti-debug` are skipped because Wasm
  requires structured control flow and has no native debugger surface.

- **iOS / Android** — see [Integration](../integration/index.md) for build-system
  wiring (Xcode, Gradle / NDK).
