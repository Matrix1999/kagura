# kagura — Swift Package Manager Integration

A Swift Package that exposes the Kagura **runtime library** as a Swift / C
target. Use it from Swift or mixed Swift + ObjC projects without CocoaPods or
manual xcconfig wiring.

## Files

| File | Purpose |
|:-----|:--------|
| `Package.swift` | Declares the `KaguraRuntime` product (iOS 13+, macOS 11+, tvOS 13+, watchOS 7+) |

---

## Usage

Add the package to your own `Package.swift`:

```swift
let package = Package(
    name: "MyApp",
    dependencies: [
        .package(url: "https://github.com/ykus4/kagura.git", from: "0.1.0"),
    ],
    targets: [
        .target(
            name: "MyApp",
            dependencies: [
                .product(name: "KaguraRuntime", package: "kagura"),
            ]
        ),
    ]
)
```

Or from Xcode → **File → Add Package Dependencies…** and paste the repo URL.

---

## What this gives you

Only the **runtime** library is shipped via SPM (anti-debug, jailbreak
detection, AES, VM interpreter, anti-cheat helpers). The compiler plugin
(`libKaguraObfuscator.dylib`) is **not** distributed via SPM — SPM does not
have first-class support for clang pass plugins of this type.

To enable obfuscation, you still need to load the plugin through Xcode build
settings:

```
OTHER_CFLAGS      = $(inherited) -fpass-plugin=$(KAGURA_PLUGIN_PATH) -mllvm -kagura-fla …
OTHER_SWIFT_FLAGS = $(inherited) -Xcc -fpass-plugin=$(KAGURA_PLUGIN_PATH) -Xcc -mllvm -Xcc -kagura-fla …
```

See [Xcode Integration](https://ykus4.github.io/kagura/integration/xcode/) for
the full xcconfig and build-phase setup.

---

## Platforms

| Platform | Minimum |
|:---------|:--------|
| iOS      | 13.0    |
| macOS    | 11.0    |
| tvOS     | 13.0    |
| watchOS  | 7.0     |

## Source layout

`Package.swift` enumerates the runtime sources explicitly. Android / Linux
sources are still listed but compile to **empty translation units on Apple
platforms** via `#ifdef __ANDROID__` / `#ifdef __linux__` guards, so they are
harmless to include.
