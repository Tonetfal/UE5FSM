#include "MachineState_Test.h"

FOnMachineActionSignature ActionDelegate;
TArray<FMachineActionPair> Actions;

void StartNewTest()
{
	// Clear old results
	Actions.Empty();

	// Listen for actions
	if (!ActionDelegate.IsBound())
	{
		ActionDelegate.BindLambda([] (TSubclassOf<UMachineState> MachineState, EStateAction StateAction)
		{
			Actions.Add({ MachineState, StateAction });
		});
	}
}
