# Regression Corpus (4.8.7)

This directory contains a regression corpus of known-attack reproduction tests.
Each test verifies that a specific past vulnerability or crash has been fixed
and does not regress.

## Structure

```
tests/regression/
├── README.md                 (this file)
├── CMakeLists.txt            (registers all regression tests with CTest)
├── corpus/
│   ├── crash_00001.ll        (LLVM IR that previously crashed a pass)
│   ├── crash_00002.ll
│   └── ...
└── scripts/
    ├── run_corpus.sh         (run all corpus entries through every pass)
    └── add_crash.sh          (add a new crash to the corpus)
```

## Adding a new regression

When a crash is found (via fuzzing, CI failure, or field report):

```bash
# 1. Minimize the reproducer
llvm-reduce --test=./scripts/test_crash.sh <original.ll> -o crash_00N.ll

# 2. Add to corpus
./scripts/add_crash.sh crash_00N.ll "Brief description of the crash"

# 3. Verify it reproduces and is now fixed
ctest -R regression_ --output-on-failure
```

## Corpus entries

| ID    | Pass  | Description |
|:------|:------|:------------|
| 00001 | fla   | PHI node after FLA causes invalid IR when entry block has predecessors |
| 00002 | bcf   | BCF on function with single BB causes infinite loop in opaque predicate |
| 00003 | sub   | SUB crashes on vector operations (no integer type check) |
| 00004 | lt    | LoopTransform on LLVM 17: insertBefore(iterator) API mismatch |
