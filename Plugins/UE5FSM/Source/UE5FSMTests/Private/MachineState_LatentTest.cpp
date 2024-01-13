#include "MachineState_LatentTest.h"

#include "MachineState_Test.h"

TCoroutine<> UMachineState_GotoStateTest1::Label_Default()
{
	// Must fail, as it's forbidden to use GotoState immediately after we enter into a label
	GOTO_STATE(UMachineState_GotoStateTest2);
	BROADCAST_TEST_MESSAGE("Goto test 2 fail", true);

	// Wait one tick to due to the reason described above
	RUN_LATENT_EXECUTION(Latent::NextTick);

	BROADCAST_TEST_MESSAGE("Pre goto test 2", true);
	GOTO_STATE(UMachineState_GotoStateTest2);

	// This is not triggered in case of successful GotoState. Note that GOTO_STATE macro co_returns in that case
	BROADCAST_TEST_MESSAGE("Post goto test 2", true);
	checkNoEntry();
}

TCoroutine<> UMachineState_GotoStateTest2::Label_Default()
{
	RUN_LATENT_EXECUTION(Latent::Seconds, 1.0);
	BROADCAST_TEST_MESSAGE("End test", true);
	co_return;
}

TCoroutine<> UMachineState_LatentExecutionTest1::Label_Default()
{
	FTimerHandle TimerHandle;
	GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::PushLatentExecutionTest2, 1.f, false);

	BROADCAST_TEST_MESSAGE("Pre sleep", true);
	RUN_LATENT_EXECUTION(Latent::Seconds, 2.0);
	BROADCAST_TEST_MESSAGE("Post sleep", true);
	BROADCAST_TEST_MESSAGE("End test", true);
}

void UMachineState_LatentExecutionTest1::PushLatentExecutionTest2()
{
	BROADCAST_TEST_MESSAGE("Pre push latent execution test 2", true);
	PushState(UMachineState_LatentExecutionTest2::StaticClass());
	BROADCAST_TEST_MESSAGE("Post push latent execution test 2", true);
}

TCoroutine<> UMachineState_LatentExecutionTest2::Label_Default()
{
	BROADCAST_TEST_MESSAGE("Pre sleep", true);
	RUN_LATENT_EXECUTION(Latent::Seconds, 3.5);
	BROADCAST_TEST_MESSAGE("Post sleep", true);
	BROADCAST_TEST_MESSAGE("Pre pop", true);
	POP_STATE();

	// This is not triggered in case of successful PopState. Note that POP_STATE macro co_returns in that case
	BROADCAST_TEST_MESSAGE("Post pop", true);
	checkNoEntry();
}
