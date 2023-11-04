#pragma once

#include "MachineState_Test.h"

#include "MachineState_LatentTest.generated.h"

UCLASS(Hidden)
class UMachineState_GotoStateTest1
	: public UMachineState_Test
{
	GENERATED_BODY()

protected:
	//~Labels
	virtual TCoroutine<> Label_Default() override;
	//~End of Labels
};

UCLASS(Hidden)
class UMachineState_GotoStateTest2
	: public UMachineState_Test
{
	GENERATED_BODY()

protected:
	//~Labels
	virtual TCoroutine<> Label_Default() override;
	//~End of Labels
};

UCLASS(Hidden)
class UMachineState_LatentExecutionTest1
	: public UMachineState_Test
{
	GENERATED_BODY()

protected:
	//~Labels
	virtual TCoroutine<> Label_Default() override;
	//~End of Labels

private:
	void PushLatentExecutionTest2();
};

UCLASS(Hidden)
class UMachineState_LatentExecutionTest2
	: public UMachineState_Test
{
	GENERATED_BODY()

protected:
	//~Labels
	virtual TCoroutine<> Label_Default() override;
	//~End of Labels
};
