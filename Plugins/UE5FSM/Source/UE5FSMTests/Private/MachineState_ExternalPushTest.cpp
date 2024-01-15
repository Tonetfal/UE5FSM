#include "MachineState_ExternalPushTest.h"

void UMachineState_ExternalPushTest::OnResumed(TSubclassOf<UMachineState> OldState)
{
	Super::OnResumed(OldState);

	FTimerHandle TimerHandle;
	GetTimerManager().SetTimer(TimerHandle, [this]
	{
		BROADCAST_TEST_MESSAGE("Prior to pop", true);
		PopState();

		// Users should NOT have any code executing after a successful pop operation, but this is just a test
		if (IsA(UMachineState_ExternalPushTest1::StaticClass()))
		{
			BROADCAST_TEST_MESSAGE("End test", true);
		}
	}, 1.f, false);
}

void UMachineState_ExternalPushTest::OnAddedToStack(EStateAction StateAction, TSubclassOf<UMachineState> OldState)
{
	Super::OnAddedToStack(StateAction, OldState);

	FTimerHandle TimerHandle;
	GetTimerManager().SetTimer(TimerHandle, [this]
	{
		if (IsValid(StateToPush))
		{
			BROADCAST_TEST_MESSAGE("Prior to push", true);
			PushState(StateToPush);
		}
		else
		{
			BROADCAST_TEST_MESSAGE("Prior to pop", true);
			PopState();
		}
	}, 1.f, false);
}

TCoroutine<> UMachineState_ExternalPushTest::Label_Default()
{
	while (true)
	{
		BROADCAST_TEST_MESSAGE("Hello", true);
		RUN_LATENT_EXECUTION(Latent::Seconds, 0.4f);
	}
}
