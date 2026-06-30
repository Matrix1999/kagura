# kagura — Bazel Integration

Starlark macros that wrap `cc_binary` / `cc_library` with the KaguraObfuscator
clang plugin injected.

## Files

| File | Purpose |
|:-----|:--------|
| `kagura.bzl`   | Public Starlark macros (`kagura_cc_binary`, `kagura_cc_library`) |
| `BUILD.bazel`  | Exposes the runtime as a `cc_library` target Bazel rules can depend on |

---

## Prerequisites

In your `WORKSPACE` (or `MODULE.bazel`):

```python
local_repository(
    name = "kagura",
    path = "/path/to/kagura",          # or use http_archive / git_repository
)

# Point Bazel at the pre-built plugin
new_local_repository(
    name = "kagura_prebuilt",
    path = "/path/to/kagura/build/lib/Transforms",
    build_file_content = """
cc_import(
    name = "libKaguraObfuscator",
    shared_library = "KaguraObfuscator.dylib",  # or .so on Linux
    visibility = ["//visibility:public"],
)
""",
)
```

---

## Usage

```python
load("@kagura//integration/bazel:kagura.bzl",
     "kagura_cc_binary", "kagura_cc_library")

kagura_cc_binary(
    name = "my_binary",
    srcs = ["main.cc"],
    kagura_passes = ["-kagura-fla", "-kagura-sub", "-kagura-str"],
    kagura_config = "//path/to:policy.json",  # optional
)

kagura_cc_library(
    name = "my_lib",
    srcs = ["lib.cc"],
    hdrs = ["lib.h"],
    # uses the default pass set if kagura_passes is omitted:
    #   ["-kagura-fla", "-kagura-str", "-kagura-sub"]
)
```

### Arguments

| Arg | Default | Meaning |
|:----|:--------|:--------|
| `kagura_passes` | `["-kagura-fla", "-kagura-str", "-kagura-sub"]` | List of `-kagura-*` flag strings to enable |
| `kagura_config` | `None` | Optional label for a JSON policy file (see [Configuration](https://ykus4.github.io/kagura/configuration/)) |

All other `cc_binary` / `cc_library` arguments (`srcs`, `hdrs`, `deps`,
`copts`, …) are forwarded as-is.

---

## How it works

The macros add two things to the underlying `cc_*` rule:

1. **`copts`** — prepends `-fpass-plugin=$(location @kagura_prebuilt//:libKaguraObfuscator)` plus every `-kagura-*` flag you requested
2. **`deps`** — adds `@kagura//integration/bazel:kagura_runtime` so the runtime
   library gets linked, and the plugin target so Bazel makes its path available
   to `$(location)`

If `kagura_config` is set, it is also added to `deps` so the file is staged
into the sandbox and resolvable via `$(location ...)`.
