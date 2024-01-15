#include "MachineState_LatentActions.h"

void UMachineState_LatentActions1::OnBegan(TSubclassOf<UMachineState> OldState)
{
	Super::OnBegan(OldState);

	GotoState(UMachineState_LatentActions2::StaticClass());
	GotoState(UMachineState_LatentActions2::StaticClass());
	GotoState(UMachineState_LatentActions2::StaticClass());
	GotoState(UMachineState_LatentActions2::StaticClass());
}
