using UnrealBuildTool;

public class OSFEditorTarget : TargetRules
{
    public OSFEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;

        DefaultBuildSettings = BuildSettingsVersion.V2;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        ExtraModuleNames.Add("OSF");
    }
}
