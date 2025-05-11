using UnrealBuildTool;

public class UE5FSM_AdditionalCoroutine : ModuleRules
{
    public UE5FSM_AdditionalCoroutine(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "AIModule",
                "Core",
                "UE5Coro",
                "UE5CoroAI",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
            }
        );
    }
}