# Frida Script Resistance Evaluation (4.8.6)

This directory contains Frida scripts and evaluation tooling to test kagura's
resistance against common Frida-based dynamic instrumentation attacks.

## Attack categories evaluated

| ID  | Attack | kagura defense |
|:----|:-------|:---------------|
| F1  | `Interceptor.attach` on string decrypt stubs | STR/STR-AES: each decrypt stub has a unique address; FLA makes location unpredictable |
| F2  | `Memory.scan` for decrypted string plaintext | Short-lived buffer (zero_buf.c): strings zeroed immediately after use |
| F3  | `Module.findExportByName` on obfuscated symbols | SymbolVisibility pass hides non-public exports |
| F4  | Frida gadget injection via `DYLD_INSERT_LIBRARIES` | AntiDebug detects DYLD_INSERT, anti_debug.c port 27042 check |
| F5  | `ObjC.classes` inspection for renamed selectors | ObjCObfuscation renames selectors in IR metadata |
| F6  | `Java.use` to hook JNI glue functions | JNIObfuscation: static `Java_*` symbols replaced with dynamic RegisterNatives |
| F7  | Stalker trace to recover FLA dispatcher logic | BCF opaque predicates increase trace entropy |
| F8  | `Memory.readByteArray` on VM bytecode blob | VMObfuscation: bytecode XOR-encrypted at rest |

## Running the evaluation

Prerequisites:
- `frida-tools` installed: `pip install frida-tools`
- A device/simulator with the test binary running
- kagura plugin built and available

```bash
# Build the test subject with kagura obfuscation
clang -O2 \
  -fpass-plugin=../../build/lib/libKaguraObfuscator.dylib \
  -kagura-str -kagura-fla -kagura-sub -kagura-sv \
  -o /tmp/kagura_frida_test \
  frida_test_subject.c

# Run the Frida probe scripts against the test binary
for script in probes/F*.js; do
  echo "--- Running $script ---"
  frida -l "$script" -f /tmp/kagura_frida_test --no-pause 2>&1
done
```

## Probe scripts

- `probes/F1_intercept_decrypt.js` — tries to hook every exported function
  containing "decrypt" or "decode" in its name.
- `probes/F2_memory_scan_strings.js` — scans all RW pages for known plaintext.
- `probes/F3_find_exports.js` — enumerates module exports and compares counts.
- `probes/F4_gadget_inject.js` — checks whether DYLD_INSERT was intercepted.
- `probes/F7_stalker_trace.js` — stalks 1000 instructions and reports unique
  addresses (FLA increases this count significantly).

## Interpreting results

Each probe prints a verdict:
- `RESISTANT` — kagura defense blocked the attack.
- `PARTIAL` — attack succeeded but with reduced information.
- `VULNERABLE` — attack succeeded without obstruction.

Results are saved to `results/<timestamp>.json` for trend tracking.
