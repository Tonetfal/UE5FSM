#pragma once

#include "FiniteStateMachine/FiniteStateMachine.h"

#include "MachineState_Test.generated.h"

UCLASS()
class UMachineState_TestGlobal
	: public UGlobalMachineState
{
	GENERATED_BODY()
};

DECLARE_DELEGATE_TwoParams(
	FOnMachineActionSignature,
	TSubclassOf<UMachineState> MachineState,
	EStateAction StateAction);

struct FMachineActionPair { TSubclassOf<UMachineState> Class; EStateAction Action; };

// Defined inside MachineState_Test.cpp
extern FOnMachineActionSignature ActionDelegate;
extern TArray<FMachineActionPair> Actions;

void StartNewTest();

// Check whether the state actions were in the same order as defined prior to test
TPair<FString, bool> CompareActions(const TArray<TPair<TSubclassOf<UMachineState>, EStateAction>>& RhsActions);

UCLASS(Abstract)
class UMachineState_Test
	: public UMachineState
{
	GENERATED_BODY()

protected:
	virtual void Begin(TSubclassOf<UMachineState> PreviousState) override
	{
		Super::Begin(PreviousState);
		ActionDelegate.Execute(GetClass(), EStateAction::Begin);
	}

	virtual void End(TSubclassOf<UMachineState> NewState) override
	{
		Super::End(NewState);
		ActionDelegate.Execute(GetClass(), EStateAction::End);
	}

	virtual void Pushed() override
	{
		Super::Pushed();
		ActionDelegate.Execute(GetClass(), EStateAction::Push);
	}

	virtual void Popped() override
	{
		Super::Popped();
		ActionDelegate.Execute(GetClass(), EStateAction::Pop);
	}

	virtual void Paused() override
	{
		Super::Paused();
		ActionDelegate.Execute(GetClass(), EStateAction::Pause);
	}

	virtual void Resumed() override
	{
		Super::Resumed();
		ActionDelegate.Execute(GetClass(), EStateAction::Resume);
	}
};

UCLASS()
class UMachineState_Test1
	: public UMachineState_Test
{
	GENERATED_BODY()
};

UCLASS()
class UMachineState_Test2
	: public UMachineState_Test
{
	GENERATED_BODY()
};

UCLASS()
class UMachineState_Test3
	: public UMachineState_Test
{
	GENERATED_BODY()
};

