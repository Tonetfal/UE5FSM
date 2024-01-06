#pragma once

#include "MachineState_Test.h"

#include "MachineState_ExternalPushPopTest.generated.h"

UCLASS(Abstract, Hidden)
class UMachineState_ExternalPushPopTest
	: public UMachineState_Test
{
	GENERATED_BODY()

protected:
	virtual void Begin(TSubclassOf<UMachineState> PreviousState) override;
	virtual void Pushed() override;
	virtual void Paused() override;
	//~Labels
	virtual TCoroutine<> Label_Default() override;
	//~End of Labels
};

UCLASS(Hidden)
class UMachineState_ExternalPushPopTest1
	: public UMachineState_ExternalPushPopTest
{
	GENERATED_BODY()
};

UCLASS(Hidden)
class UMachineState_ExternalPushPopTest2
	: public UMachineState_ExternalPushPopTest
{
	GENERATED_BODY()
};

UCLASS(Hidden)
class UMachineState_ExternalPushPopTest3
	: public UMachineState_ExternalPushPopTest
{
	GENERATED_BODY()
};
