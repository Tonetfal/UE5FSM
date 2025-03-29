#pragma once

#if !WITH_GAMEPLAY_DEBUGGER
	#undef WITH_UE5FSM_DEBUGGER
#endif

#ifdef WITH_UE5FSM_DEBUGGER

#include "FiniteStateMachine/FiniteStateMachine.h"
#include "FiniteStateMachine/MachineState.h"
#include "GameplayDebuggerCategory.h"

class UMachineState;

struct FSerializedStateData
{
public:
	FString Name = "None";
	EStateAction LastAction = EStateAction::None;
	float TimeSinceLastStateAction = 0.f;
	FString ExtDebugData = "";
};

struct FSerializedFSMData
{
public:
	TSubclassOf<UMachineState> GlobalStateClass = nullptr;
	TArray<TSubclassOf<UMachineState>> RegisteredStateClasses;
	TArray<FSerializedStateData> StatesStack;
	TArray<UFiniteStateMachine::FDebugStateAction> LastTerminatedStates;
	FString ExtGlobalDebugData = "";
};

class FGameplayDebuggerCategory_UE5FSM
	: public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_UE5FSM();

	/** Creates an instance of this category - will be used on module startup to include our category in the Editor */
	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	//~FGameplayDebuggerCategory Interface
	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;
	//~End of FGameplayDebuggerCategory Interface

private:
	TArray<FSerializedFSMData> DebugData;
};

#endif
