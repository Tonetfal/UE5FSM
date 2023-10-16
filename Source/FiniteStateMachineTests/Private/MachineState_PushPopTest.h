#pragma once

#include "MachineState_Test.h"

#include "MachineState_PushPopTest.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateMachine_Label_Test);

struct FPushPopLatentMessage { TSubclassOf<UMachineState> Class; FString Message; bool bSuccess; };
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMachineMessage, FPushPopLatentMessage);

// Defined inside MachineState_PushPopTest.cpp
extern FOnMachineMessage OnMessageDelegate;

UCLASS(Abstract, Hidden)
class UMachineState_PushPopTest
	: public UMachineState_Test
{
	GENERATED_BODY()

public:
	UMachineState_PushPopTest();

protected:
	//~Labels
	virtual TCoroutine<> Label_Default() override;
	TCoroutine<> Label_Test();
	//~End of Labels

protected:
	TSubclassOf<UMachineState_PushPopTest> LatentPushState;
	bool bLatentPopState = true;
	bool bNotifyTestFinish = false;
};

UCLASS()
class UMachineState_PushPopTest3
	: public UMachineState_PushPopTest
{
	GENERATED_BODY()

public:
	UMachineState_PushPopTest3()
	{
		LatentPushState = nullptr;
	}
};

UCLASS()
class UMachineState_PushPopTest2
	: public UMachineState_PushPopTest
{
	GENERATED_BODY()

public:
	UMachineState_PushPopTest2()
	{
		LatentPushState = UMachineState_PushPopTest3::StaticClass();
	}
};

UCLASS()
class UMachineState_PushPopTest1
	: public UMachineState_PushPopTest
{
	GENERATED_BODY()

public:
	UMachineState_PushPopTest1()
	{
		LatentPushState = UMachineState_PushPopTest2::StaticClass();
		bNotifyTestFinish = true;
	}
};
