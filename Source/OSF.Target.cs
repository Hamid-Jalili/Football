using UnrealBuildTool;
using System.Collections.Generic;

public class OSFTarget : TargetRules
{
    public OSFTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6; // <- update
        ExtraModuleNames.AddRange(new string[] { "OSF" });
    }
}
