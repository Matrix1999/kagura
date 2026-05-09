// swift-tools-version: 5.9
//
// 4.6.12: Swift Package Manager support for KaguraRuntime.
//
// This package exposes the kagura runtime C library as a Swift/C target so
// that Swift and mixed Swift+ObjC projects can link it without CocoaPods or
// manual xcconfig wiring.
//
// Usage in Package.swift:
//   .package(url: "https://github.com/ykus4/kagura.git", from: "0.1.0")
//
// Then add to your target:
//   .target(
//       name: "MyApp",
//       dependencies: [
//           .product(name: "KaguraRuntime", package: "kagura"),
//       ]
//   )
//
// The LLVM pass plugin (libKaguraObfuscator.dylib) must still be loaded via
// the Xcode build system (OTHER_SWIFT_FLAGS / OTHER_CFLAGS) since SPM does
// not have first-class support for compiler plugins of this type.  See
// integration/xcode/README.md for instructions.
//
// Note: Android JNI sources are excluded at the source level via conditional
// compilation (#ifdef __ANDROID__); they compile to empty translation units
// on Apple platforms and are harmless to include.

import PackageDescription

let package = Package(
    name: "kagura",
    platforms: [
        .iOS(.v13),
        .macOS(.v11),
        .tvOS(.v13),
        .watchOS(.v7),
    ],
    products: [
        .library(
            name: "KaguraRuntime",
            targets: ["KaguraRuntime"]
        ),
    ],
    targets: [
        .target(
            name: "KaguraRuntime",
            path: ".",
            sources: [
                "runtime/anti_debug.c",
                "runtime/aes.c",
                "runtime/il2cpp_protection.c",
                "runtime/jailbreak_detection.c",
                "runtime/vm_interpreter.c",
                "runtime/hook_detection.c",
                "runtime/breakpoint_detection.c",
                "runtime/emulator_detection.c",
                "runtime/zero_buf.c",
                "runtime/macho_integrity.c",
                "runtime/ios_integrity.c",
                "runtime/ios_jailbreak_advanced.c",
                "runtime/loaded_library_scan.c",
                "runtime/symbol_interposition.c",
                "runtime/direct_syscall.c",
                "runtime/ios_platform.c",
                "runtime/blob_integrity.c",
                "runtime/fishhook_countermeasure.c",
                "runtime/testflight_detect.c",
                "runtime/state_integrity.c",
                "runtime/behavior_log.c",
                "runtime/game_values.c",
                "runtime/soft_response.c",
                "runtime/device_key.c",
                "runtime/crash_symbolication.c",
                // Android-guarded files below compile to empty TUs on Apple.
                "runtime/jni_hook_detection.c",
                "runtime/play_integrity.c",
                "runtime/safetynet_compat.c",
                "runtime/art_environment.c",
                "runtime/proc_inspection.c",
                "runtime/seccomp_checks.c",
                "runtime/load_order.c",
            ],
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("include"),
                .define("KAGURA_SWIFTPM"),
            ],
            linkerSettings: [
                // Required for dlopen/dladdr (crash symbolication, anti-debug)
                .linkedLibrary("dl", .when(platforms: [.macOS, .linux])),
            ]
        ),
    ],
    cLanguageStandard: .c11
)
