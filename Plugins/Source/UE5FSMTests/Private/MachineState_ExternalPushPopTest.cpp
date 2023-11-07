#include "MachineState_ExternalPushPopTest.h"

void UMachineState_ExternalPushPopTest::Begin(TSubclassOf<UMachineState> PreviousState)
{
	Super::Begin(PreviousState);
}

void UMachineState_ExternalPushPopTest::Pushed()
{
	Super::Pushed();
}

void UMachineState_ExternalPushPopTest::Paused()
{
	Super::Paused();
}

TCoroutine<> UMachineState_ExternalPushPopTest::Label_Default()
{
	co_await RUN_LATENT_EXECUTION(Latent::Seconds, 0.5);
	BROADCAST_TEST_MESSAGE("Post default label", true);
}
