using UnrealBuildTool;

public class UE5FSM : ModuleRules
{
    public UE5FSM(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "GameplayTags",
                "UE5Coro",
                "UE5CoroAI",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "GameplayDebugger",
            }
        );

        // Enable debug for UE5FSM using gameplay debugger (default binding is apostrophe ('))
        PublicDefinitions.Add("WITH_UE5FSM_DEBUGGER");
    }
}
