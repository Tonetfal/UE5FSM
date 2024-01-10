#pragma once

#include "FiniteStateMachine/FiniteStateMachine.h"

#include "MachineState_Test.generated.h"

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
	virtual void OnBegan(TSubclassOf<UMachineState> PreviousState) override
	{
		Super::OnBegan(PreviousState);
		BROADCAST_TEST_MESSAGE("Begin", true);
	}

	virtual void OnEnded(TSubclassOf<UMachineState> NewState) override
	{
		Super::OnEnded(NewState);
		BROADCAST_TEST_MESSAGE("End", true);
	}

	virtual void OnPushed() override
	{
		Super::OnPushed();
		BROADCAST_TEST_MESSAGE("Pushed", true);
	}

	virtual void OnPopped() override
	{
		Super::OnPopped();
		BROADCAST_TEST_MESSAGE("Popped", true);
	}

	virtual void OnPaused() override
	{
		Super::OnPaused();
		BROADCAST_TEST_MESSAGE("Paused", true);
	}

	virtual void OnResumed() override
	{
		Super::OnResumed();
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

