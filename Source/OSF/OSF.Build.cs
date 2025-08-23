using UnrealBuildTool;

public class OSF : ModuleRules
{
    public OSF(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine",
            "InputCore", "PhysicsCore", "Chaos"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate", "SlateCore"
        });
    }
}
