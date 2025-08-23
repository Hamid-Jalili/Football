// OSF.Target.cs

using UnrealBuildTool;
using System.Collections.Generic;

public class OSFTarget : TargetRules
{
    public OSFTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;

        // ✅ Use the latest UE5.6 defaults
        DefaultBuildSettings = BuildSettingsVersion.V5;

        // ✅ Explicit C++20 (UE5.6 no longer supports C++17)
        CppStandard = CppStandardVersion.Cpp20;

        // ✅ Allow overrides in installed engine (instead of Unique)
        bOverrideBuildEnvironment = true;

        // Example: if you want to keep strict off (optional)
        WindowsPlatform.bStrictConformanceMode = false;

        ExtraModuleNames.AddRange(new string[] { "OSF" });
    }
}
