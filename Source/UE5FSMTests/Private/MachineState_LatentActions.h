#pragma once

#include "MachineState_Test.h"

#include "MachineState_LatentActions.generated.h"

UCLASS(Hidden)
class UMachineState_LatentActions1
	: public UMachineState_Test
{
	GENERATED_BODY()

protected:
	//~UMachineState_Test Interface
	virtual void OnBegan(TSubclassOf<UMachineState> OldState) override;
	//~End of UMachineState_Test Interface
};

UCLASS(Hidden)
class UMachineState_LatentActions2
	: public UMachineState_Test
{
	GENERATED_BODY()

};
