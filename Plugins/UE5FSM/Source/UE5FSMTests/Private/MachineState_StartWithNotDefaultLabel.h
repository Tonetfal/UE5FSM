#pragma once

#include "MachineState_Test.h"

#include "MachineState_StartWithNotDefaultLabel.generated.h"

/**
 *
 */
UCLASS()
class UMachineState_StartWithNotDefaultLabel
	: public UMachineState_Test
{
	GENERATED_BODY()

public:
	UMachineState_StartWithNotDefaultLabel();

protected:
	//~UMachineState_Test Interface
	virtual void OnBegan(TSubclassOf<UMachineState> PreviousState) override;
	//~End of UMachineState_Test Interface

	//~Labels
	virtual TCoroutine<> Label_Default() override;
	TCoroutine<> Label_Test();
	//~End of Labels
};
