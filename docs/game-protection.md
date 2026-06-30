# Game Protection

`include/kagura/game_protect.h` provides a C++17 header-only `Protected<T>`
template for protecting game-critical values (HP, damage, currency, etc.)
from memory scanners (Cheat Engine, GameGuardian) and value-freeze tools.

This is a **run-time** complement to the [`kagura-mvo`](passes/data.md) and
[`kagura-pe`](passes/data.md) compile-time passes — use both together for
maximum coverage.

## Usage

```cpp
#include "kagura/game_protect.h"

kagura::Protected<int>   hp(100);
kagura::Protected<float> speed(5.5f);

hp -= 30;
if (hp <= 0) die();

// Optional tamper callback (default: spin forever to deny a clean crash point)
kagura::Protected<int>::setTamperCallback([]{ report_cheat(); });
```

Convenience aliases: `ProtectedInt`, `ProtectedFloat`, `ProtectedDouble`,
`ProtectedInt64`, etc.

## Protection strategy

- Value is stored XOR-encrypted with a **per-instance runtime key** seeded by
  ASLR + stack entropy — every instance has a different mask.
- A **shadow copy** with a different XOR mask lets the template detect external
  writes (memory editor pokes) on every read.
- The plaintext value is **never resident in memory** between reads and writes;
  it exists only in registers during the operator body.

## Pairing with passes

| Layer | Protects against |
|:------|:-----------------|
| `Protected<T>` (runtime) | Memory scanners, value-freeze tools, scripted pokes |
| `kagura-mvo` (compile-time) | Static value extraction from stack frames |
| `kagura-pe` (compile-time)  | Pointer chasing in dumped heap snapshots |
| `kagura-anti-debug` (runtime) | Cheat Engine attached as debugger, Frida injection |
