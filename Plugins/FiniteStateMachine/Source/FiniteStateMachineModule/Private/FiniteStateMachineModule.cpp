#include "FiniteStateMachineModule.h"

#include "GameplayDebugger.h"
#include "FiniteStateMachine/Debug/GameplayDebuggerCategory_UE5FSM.h"

#define LOCTEXT_NAMESPACE "FFiniteStateMachineModule"

const static FName DebuggerCategoryName = "UE5FSM";

void FFiniteStateMachineModule::StartupModule()
{
#if WITH_UE5FSM_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		const auto* CategoryInstance = &FGameplayDebuggerCategory_UE5FSM::MakeInstance;
		const auto Category = IGameplayDebugger::FOnGetCategory::CreateStatic(CategoryInstance);

		auto& GameplayDebuggerModule = IGameplayDebugger::Get();
		GameplayDebuggerModule.RegisterCategory(DebuggerCategoryName, Category,
			EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
		GameplayDebuggerModule.NotifyCategoriesChanged();
	}
#endif
}

void FFiniteStateMachineModule::ShutdownModule()
{
#if WITH_UE5FSM_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
		GameplayDebuggerModule.UnregisterCategory(DebuggerCategoryName);
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFiniteStateMachineModule, FiniteStateMachineModule)
