# Contributing to kagura

## Getting Started

```bash
git clone https://github.com/yotti/kagura.git
cd kagura
bash build.sh
cd build && ctest --output-on-failure
```

## Adding a Pass

1. Add the pass declaration to `include/kagura/Passes.h`
2. Implement in `lib/Transforms/YourPass.cpp`
3. Register the source in `lib/Transforms/CMakeLists.txt`
4. Register the pass in `lib/Transforms/Plugin.cpp`
   - Add a `cl::opt<bool>` flag
   - Register via `registerPipelineParsingCallback`
   - Wire up the `registerOptimizerLastEPCallback` if needed
5. Add integration test input to `tests/inputs/`
6. Add a `ctest` entry in `tests/CMakeLists.txt`
7. Add a FileCheck test in `tests/lit/`

## Pass Guidelines

- Use `PassInfoMixin` (New Pass Manager only — no legacy pass support)
- Skip declarations: `if (F.isDeclaration()) return PA;`
- Skip functions with `__attribute__((optnone))` or `noinline` when appropriate
- Prefer `kagura::PRNG` from `Utils.h` for all randomness; respect `-kagura-seed`
- Do not break exception handling — skip functions containing `invoke` or `landingpad` when the pass cannot handle them
- Keep `isRequired()` returning `false` for all obfuscation passes

## Runtime Library

If your pass needs runtime support, add a `.c` file under `runtime/` and declare the runtime function as `extern` in the pass. Register the source in `runtime/CMakeLists.txt`.

## Tests

- **Integration tests** (`tests/inputs/` + `tests/CMakeLists.txt`): verify the pass compiles without crashing
- **FileCheck tests** (`tests/lit/`): verify specific IR transformations using `.ll` inputs

All tests must pass across LLVM 17, 18, and 19.

## Code Style

- Follow LLVM coding conventions (camelCase for functions, PascalCase for types)
- File header comment format:
  ```
  //===-- YourPass.cpp - Short description ---------------------------------===//
  ```
- No `using namespace std`

## Pull Requests

- One pass or feature per PR
- Include a FileCheck test that fails before the change and passes after
- CI must be green before merge

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
