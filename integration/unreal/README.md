# kagura — Unreal Engine Integration

Applies kagura obfuscation to UE native code (C++) for Android and iOS shipping builds.

## Prerequisites

- Unreal Engine 5.1 or later
- kagura built: run `bash build.sh` from the repo root
- Build targets: Android or iOS (Shipping configuration)

## Installation

### 1. Add the UBT toolchain

Copy `KaguraToolchain.cs` to your project or engine:

```
YourProject/
  Source/
    Programs/
      UnrealBuildTool/
        KaguraToolchain.cs   ← here
```

Or for an engine-wide install:
```
Engine/Source/Programs/UnrealBuildTool/Platform/Android/KaguraToolchain.cs
```

### 2. Add the module build rules (optional, for runtime linking)

Copy `KaguraObfuscation.Build.cs` to your project's `Source/ThirdParty/` directory
and add the module to your game module's `.Build.cs`:

```csharp
// YourGame.Build.cs
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine",
    "KaguraObfuscation"   // ← add this
});
```

### 3. Set environment variables

```bash
export KAGURA_PLUGIN_PATH=/path/to/build/lib/Transforms/KaguraObfuscator.dylib
export KAGURA_RUNTIME_LIB=/path/to/build/runtime/libkagura_runtime.a
```

Or edit the defaults in `KaguraConfig` inside `KaguraToolchain.cs`.

### 4. Activate in your Target.cs

```csharp
// YourGameTarget.cs
public class YourGameTarget : TargetRules
{
    public YourGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        // kagura toolchain is activated automatically for Android/iOS Shipping
        // when KaguraToolchain.cs is present in the UBT search path.
    }
}
```

## Configuration

Edit `KaguraConfig` in `KaguraToolchain.cs`:

```csharp
public static class KaguraConfig
{
    public static bool EnableStr       = true;
    public static bool EnableFla       = true;
    public static bool EnableBcf       = true;
    public static bool ShippingOnly    = true;  // only apply for Shipping builds
    public static int  BcfProb         = 30;
    // ... see file for full list
}
```

## Pass reference

See the main [README](../../README.md#features) for the full pass list and options.

## Build verification

After building, run:

```bash
nm -D YourGame.so | grep "your_sensitive_symbol"
```

With `-kagura-sv` enabled, internal symbols should not appear in the dynamic symbol table.
