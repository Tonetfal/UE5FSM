using UnrealBuildTool;

public class FiniteStateMachineTests : ModuleRules
{
    public FiniteStateMachineTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "FiniteStateMachineModule",
                "GameplayTags",
                "UE5Coro",
                "UnrealEd",
            }
        );
    }
}