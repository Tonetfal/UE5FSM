#include "MachineState_PushPopTest.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_StateMachine_Label_Test, "StateMachine.Label.Test");

UMachineState_PushPopTest::UMachineState_PushPopTest()
{
	REGISTER_LABEL(Test);
}

void UMachineState_PushPopTest::Popped()
{
	Super::Popped();

	if (bNotifyTestFinish)
	{
		BROADCAST_TEST_MESSAGE("End test", true);
	}
}

TCoroutine<> UMachineState_PushPopTest::Label_Default()
{
	// THAT IS NOT THE WAY USERS ARE SUPPOSED TO USE THAT
	// Usually after GotoLabel we end the execution of the label, however for test purposes we're going to do something
	// after that
	if (GOTO_LABEL(Test))
	{
		// Label_Test is activated only the next tick, hence it'll execute the code below before that

		FTimerHandle TimerHandle;
		GetTimerManager().SetTimer(TimerHandle, [this]
		{
			BROADCAST_TEST_MESSAGE("Timer delay works", true);
		}, 1.5f, false);

		// Should be cancelled by the test label
		co_await RUN_LATENT_EXECUTION(Latent::Seconds, 20.0);
		BROADCAST_TEST_MESSAGE("Latent execution has been cancelled", true);
	}
}

TCoroutine<> UMachineState_PushPopTest::Label_Test()
{
	if (bCancelLatentExecution)
	{
		StopLatentExecution();
	}

	if (LatentPushState)
	{
		co_await RUN_LATENT_EXECUTION(Latent::Seconds, 1.0);
		co_await PUSH_STATE_CLASS(LatentPushState);
	}

	co_await RUN_LATENT_EXECUTION(Latent::Seconds, 1.0);
	POP_STATE();
}
