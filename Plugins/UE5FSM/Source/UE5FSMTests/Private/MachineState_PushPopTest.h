#pragma once

#include "MachineState_Test.h"

#include "MachineState_PushPopTest.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateMachine_Label_Test);

UCLASS(Abstract, Hidden)
class UMachineState_PushPopTest
	: public UMachineState_Test
{
	GENERATED_BODY()

public:
	UMachineState_PushPopTest();

protected:
	//~UMachineState_Test Interface
	virtual void OnPopped(TSubclassOf<UMachineState> NewState) override;
	//~End of UMachineState_Test Interface

	//~Labels
	virtual TCoroutine<> Label_Default() override;
	TCoroutine<> Label_Test();
	//~End of Labels


protected:
	TSubclassOf<UMachineState_PushPopTest> LatentPushState = nullptr;
	bool bNotifyTestFinish = false;
	bool bCancelLatentExecution = false;
};

UCLASS(Hidden)
class UMachineState_PushPopTest3
	: public UMachineState_PushPopTest
{
	GENERATED_BODY()

public:
	UMachineState_PushPopTest3()
	{
		LatentPushState = nullptr;
		bCancelLatentExecution = true;
	}
};

UCLASS(Hidden)
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

UCLASS(Hidden)
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
