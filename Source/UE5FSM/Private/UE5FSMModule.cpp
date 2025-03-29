﻿#include "UE5FSMModule.h"

#include "GameplayDebugger.h"
#include "FiniteStateMachine/Debug/GameplayDebuggerCategory_UE5FSM.h"

#define LOCTEXT_NAMESPACE "FUE5FSMModule"

UE_DEFINE_GAMEPLAY_TAG(TAG_StateMachine_Label_Test, "StateMachine.Label.Test");

const static FName DebuggerCategoryName = "UE5FSM";

void FUE5FSMModule::StartupModule()
{
#ifdef WITH_UE5FSM_DEBUGGER
	const auto* CategoryInstance = &FGameplayDebuggerCategory_UE5FSM::MakeInstance;
	const auto Category = IGameplayDebugger::FOnGetCategory::CreateStatic(CategoryInstance);

	auto& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory(DebuggerCategoryName, Category,
		EGameplayDebuggerCategoryState::Disabled);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void FUE5FSMModule::ShutdownModule()
{
#ifdef WITH_UE5FSM_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.UnregisterCategory(DebuggerCategoryName);
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUE5FSMModule, UE5FSM)
