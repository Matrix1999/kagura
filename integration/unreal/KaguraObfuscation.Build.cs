// Copyright (c) 2025 yotti. MIT License.
//
// KaguraObfuscation.Build.cs
// UE module build rules for the kagura runtime library.
//
// This module links libkagura_runtime.a into any UE module that depends on it.
// Add "KaguraObfuscation" to your module's PublicDependencyModuleNames or
// PrivateDependencyModuleNames in your own .Build.cs.
//
// Example (YourGame.Build.cs):
//   PublicDependencyModuleNames.AddRange(new string[] {
//       "Core", "CoreUObject", "Engine",
//       "KaguraObfuscation"   // ← add this
//   });

using System;
using System.IO;
using UnrealBuildTool;

public class KaguraObfuscation : ModuleRules
{
    public KaguraObfuscation(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string KaguraRoot = Path.GetFullPath(
            Path.Combine(ModuleDirectory, "../../../../")); // repo root

        string RuntimeLibDir  = Path.Combine(KaguraRoot, "build", "runtime");
        string RuntimeLibPath = Path.Combine(RuntimeLibDir, "libkagura_runtime.a");

        if (!File.Exists(RuntimeLibPath))
        {
            // Fall back to environment variable
            string envPath = Environment.GetEnvironmentVariable("KAGURA_RUNTIME_LIB");
            if (!string.IsNullOrEmpty(envPath) && File.Exists(envPath))
                RuntimeLibPath = envPath;
            else
                System.Console.Error.WriteLine(
                    "[kagura] WARNING: libkagura_runtime.a not found. " +
                    "Build kagura first or set KAGURA_RUNTIME_LIB.");
        }

        if (File.Exists(RuntimeLibPath))
        {
            // iOS / macOS
            if (Target.Platform == UnrealTargetPlatform.IOS ||
                Target.Platform == UnrealTargetPlatform.Mac)
            {
                PublicAdditionalLibraries.Add(RuntimeLibPath);
            }
            // Android (per-ABI)
            else if (Target.Platform == UnrealTargetPlatform.Android)
            {
                PublicAdditionalLibraries.Add(RuntimeLibPath);
            }
        }

        // Public include path for kagura runtime headers (if any)
        string IncludePath = Path.Combine(KaguraRoot, "include");
        if (Directory.Exists(IncludePath))
            PublicIncludePaths.Add(IncludePath);
    }
}
