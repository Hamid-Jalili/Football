// OSFEditor.Target.cs

using UnrealBuildTool;
using System.Collections.Generic;

public class OSFEditorTarget : TargetRules
{
    public OSFEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;

        // ✅ Use the latest UE5.6 defaults
        DefaultBuildSettings = BuildSettingsVersion.V5;

        // ✅ Explicit C++20 (required, C++17 dropped in UE5.6)
        CppStandard = CppStandardVersion.Cpp20;

        // ✅ Instead of Unique (which fails with installed engine),
        // allow overrides by forcing this flag:
        bOverrideBuildEnvironment = true;

        // Example: re-disable strict mode if you want
        WindowsPlatform.bStrictConformanceMode = false;

        ExtraModuleNames.AddRange(new string[] { "OSF" });
    }
}
