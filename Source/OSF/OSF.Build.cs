using UnrealBuildTool;

public class OSF : ModuleRules
{
    public OSF(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore",
            "AIModule", "GameplayTasks"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate", "SlateCore"
        });

        // No explicit include paths needed if headers are under Source/OSF/ (or /Public)
    }
}
