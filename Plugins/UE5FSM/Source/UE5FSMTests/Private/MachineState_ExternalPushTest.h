#pragma once

#include "MachineState_Test.h"

#include "MachineState_ExternalPushTest.generated.h"

/**
 *
 */
UCLASS()
class UMachineState_ExternalPushTest
	: public UMachineState_Test
{
	GENERATED_BODY()

protected:
	//~UMachineState_Test Interface
	virtual void OnResumed(TSubclassOf<UMachineState> OldState) override;
	virtual void OnAddedToStack(EStateAction StateAction, TSubclassOf<UMachineState> OldState) override;
	//~End of UMachineState_Test Interface

	//~Labels
	virtual TCoroutine<> Label_Default() override;
	//~End of Labels

protected:
	TSubclassOf<UMachineState_ExternalPushTest> StateToPush = nullptr;
};

UCLASS()
class UMachineState_ExternalPushTest3
	: public UMachineState_ExternalPushTest
{
	GENERATED_BODY()

public:
	UMachineState_ExternalPushTest3()
	{
		StateToPush = nullptr;
	}
};

UCLASS()
class UMachineState_ExternalPushTest2
	: public UMachineState_ExternalPushTest
{
	GENERATED_BODY()

public:
	UMachineState_ExternalPushTest2()
	{
		StateToPush = UMachineState_ExternalPushTest3::StaticClass();
	}
};

UCLASS()
class UMachineState_ExternalPushTest1
	: public UMachineState_ExternalPushTest
{
	GENERATED_BODY()

public:
	UMachineState_ExternalPushTest1()
	{
		StateToPush = UMachineState_ExternalPushTest2::StaticClass();
	}
};
