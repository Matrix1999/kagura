# Contributing to Kagura

## Getting Started

```bash
git clone https://github.com/ykus4/kagura.git
cd kagura
bash build.sh
cd build && ctest --output-on-failure
```

## Adding a Pass

1. Add the pass declaration to `include/kagura/Passes.h`
2. Implement in `lib/Transforms/<Subsystem>/YourPass.cpp`
3. Register the source in `lib/Transforms/CMakeLists.txt`
4. Add a `cl::opt` flag in `lib/Transforms/Options.cpp` and declare it in `include/kagura/Options.h`
5. Register in `lib/Transforms/Plugin.cpp`:
   - Named-pass callback via `registerPipelineParsingCallback`
   - Auto-injection via `registerOptimizerLastEPCallback` (if applicable)
6. Add integration test input to `tests/inputs/`
7. Add a `kagura_add_pass_test()` entry in `tests/CMakeLists.txt`
8. Add a FileCheck test in `tests/lit/<your-pass>.ll`

## Pass Guidelines

- Use `PassInfoMixin` (New Pass Manager only — no legacy pass support)
- Skip declarations: `if (F.isDeclaration()) return PA;`
- Check `shouldObfuscate(F, "passname", defaultEnabled)` from `Utils.h` to respect per-function annotations
- Skip functions with exception handling when the pass cannot handle EH: `if (hasExceptionHandling(F)) return PA;`
- Use `kagura::PRNG` from `Utils.h` for all randomness; respect `-kagura-seed`
- Keep `isRequired()` returning `false` for all obfuscation passes
- Use `kagura::getModuleTriple(M)` (not `M.getTargetTriple()` directly) for LLVM 17–22 compatibility

## Runtime Library

If your pass needs runtime support, add a `.c` file under `runtime/` and register it in `runtime/CMakeLists.txt`. Declare any runtime functions in an `extern "C"` block in the pass file.

## Tests

- **Integration tests** (`tests/inputs/` + `tests/CMakeLists.txt`): verify the pass compiles and runs without crashing on real C inputs
- **FileCheck lit tests** (`tests/lit/`): verify specific IR transformations using `.ll` inputs with `; CHECK:` directives

All tests must pass across LLVM 17, 19, 21, and 22.

To run only the FileCheck tests:

```bash
cd build && ctest -R lit-filecheck --output-on-failure
```

To run the differential tests (obfuscated vs. plain output comparison):

```bash
./scripts/differential-test.sh
```

## Code Style

- Follow LLVM coding conventions (camelCase for functions, PascalCase for types)
- File header comment format:
  ```
  //===-- YourPass.cpp - Short description ---------------------------------===//
  ```
- No `using namespace std`
- No `LLVM_VERSION_MAJOR` guards for the `getTargetTriple()` API — use `kagura::getModuleTriple()` instead

## Pull Requests

- One pass or feature per PR
- Include a FileCheck test (`.ll`) that verifies the pass transformation
- Run `./scripts/differential-test.sh` locally and confirm no regressions
- CI must be green before merge

## Release Process

Releases are published from the `main` branch. On GitHub:

1. Create a new Release tag (e.g., `v0.2.0`) from the GitHub UI.
2. The `release.yml` workflow triggers automatically and uploads pre-built binaries for:
   - macOS arm64 × LLVM 21 and 22
   - Linux x86_64 × LLVM 19 and 21
3. A source tarball (`kagura-<version>-source.tar.gz`) is also attached.

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
