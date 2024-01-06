using UnrealBuildTool;

public class UE5FSMTests : ModuleRules
{
    public UE5FSMTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "UE5Coro",
                "UE5FSM",
                "UnrealEd",
            }
        );
    }
}
