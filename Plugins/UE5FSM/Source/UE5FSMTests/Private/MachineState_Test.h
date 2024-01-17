#pragma once

#include "FiniteStateMachine/FiniteStateMachine.h"

#include "MachineState_Test.generated.h"

using namespace UE5Coro;

struct FStateMachineTestMessage { TSubclassOf<UMachineState> Class; FString Message; bool bSuccess; };
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMachineMessage, FStateMachineTestMessage);

#define BROADCAST_TEST_MESSAGE(MESSAGE, SUCCESS) OnMessageDelegate.Broadcast({ GetClass(), MESSAGE, SUCCESS })

// Defined inside MachineState_Test.cpp
extern FOnMachineMessage OnMessageDelegate;

UCLASS(Abstract, Hidden)
class UMachineState_Test
	: public UMachineState
{
	GENERATED_BODY()

protected:
	virtual void OnBegan(TSubclassOf<UMachineState> OldState) override
	{
		Super::OnBegan(OldState);
		BROADCAST_TEST_MESSAGE("Begin", true);
	}

	virtual void OnEnded(TSubclassOf<UMachineState> NewState) override
	{
		Super::OnEnded(NewState);
		BROADCAST_TEST_MESSAGE("End", true);
	}

	virtual void OnPushed(TSubclassOf<UMachineState> OldState) override
	{
		Super::OnPushed(OldState);
		BROADCAST_TEST_MESSAGE("Pushed", true);
	}

	virtual void OnPopped(TSubclassOf<UMachineState> NewState) override
	{
		Super::OnPopped(NewState);
		BROADCAST_TEST_MESSAGE("Popped", true);
	}

	virtual void OnPaused(TSubclassOf<UMachineState> OldState) override
	{
		Super::OnPaused(OldState);
		BROADCAST_TEST_MESSAGE("Paused", true);
	}

	virtual void OnResumed(TSubclassOf<UMachineState> NewState) override
	{
		Super::OnResumed(NewState);
		BROADCAST_TEST_MESSAGE("Resumed", true);
	}
};

UCLASS(Hidden)
class UMachineState_Test1
	: public UMachineState_Test
{
	GENERATED_BODY()
};

UCLASS(Hidden)
class UMachineState_Test2
	: public UMachineState_Test
{
	GENERATED_BODY()
};

UCLASS(Hidden)
class UMachineState_Test3
	: public UMachineState_Test
{
	GENERATED_BODY()
};

