#include "MachineState_PushPopTest.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_StateMachine_Label_Test, "StateMachine.Label.Test");

UMachineState_PushPopTest::UMachineState_PushPopTest()
{
	REGISTER_LABEL(Test);
}

void UMachineState_PushPopTest::OnPopped(TSubclassOf<UMachineState> NewState)
{
	Super::OnPopped(NewState);

	if (bNotifyTestFinish)
	{
		BROADCAST_TEST_MESSAGE("End test", true);
	}
}

TCoroutine<> UMachineState_PushPopTest::Label_Default()
{
	// !!! THAT IS NOT THE WAY USERS ARE SUPPOSED TO USE THAT !!!

	// After a successful GotoLabel we are meant to end the execution of the label,
	// however for test purposes we're going to do something after that.
	// Users must be using GOTO_LABEL() macro instead.

	if (GotoLabel(TAG_StateMachine_Label_Test))
	{
		// Label_Test is activated only the next tick, hence it'll execute the code below before that

		BROADCAST_TEST_MESSAGE("Before test label activation", true);

		// Should be cancelled by the test label
		RUN_LATENT_EXECUTION(Latent::Seconds, 120.0);
		BROADCAST_TEST_MESSAGE("Latent execution has been cancelled", true);
	}
}

TCoroutine<> UMachineState_PushPopTest::Label_Test()
{
	if (bCancelLatentExecution)
	{
		StopLatentExecution();
	}

	if (IsValid(LatentPushState))
	{
		RUN_LATENT_EXECUTION(Latent::Seconds, 1.0);
		PUSH_STATE_CLASS(LatentPushState);
	}

	RUN_LATENT_EXECUTION(Latent::Seconds, 1.0);
	POP_STATE();
}
