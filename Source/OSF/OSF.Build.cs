// OSF.Build.cs

using UnrealBuildTool;

public class OSF : ModuleRules
{
    public OSF(ReadOnlyTargetRules Target) : base(Target)
    {
        // ✅ UE 5.6 modern PCH handling
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // ✅ C++20 standard (matches OSFEditor.Target.cs)
        CppStandard = CppStandardVersion.Cpp20;

        // ✅ Public dependencies (accessible to other modules)
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "PhysicsCore",     // Physics abstraction layer
            "Chaos",           // Chaos physics (ball/characters)
            "UMG",             // UI framework
            "Slate",
            "SlateCore",
            "AIModule",        // AI controllers
            "NavigationSystem",
            "EnhancedInput"    // UE5 input system
        });

        // ✅ Private dependencies (used internally only)
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "NetCore",
            "Networking",
            "Sockets"
        });

        // ✅ Enforce IWYU (include-what-you-use) for UE5.6 strict conformance
        bEnforceIWYU = true;

        // ✅ Explicit module settings for UE 5.6
        // Makes sure we don't inherit old UE4/5.0 behaviors
        DefaultBuildSettings = BuildSettingsVersion.V5;
    }
}
