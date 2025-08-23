using UnrealBuildTool;

public class OSFTarget : TargetRules
{
    public OSFTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;

        // UE 5.3: use V2 or Latest (V5 does not exist here)
        DefaultBuildSettings = BuildSettingsVersion.V2;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        ExtraModuleNames.Add("OSF");
    }
}
