#include "MachineState_PushPopTest.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_StateMachine_Label_Test, "StateMachine.Label.Test");

FOnMachineMessage OnMessageDelegate;

UMachineState_PushPopTest::UMachineState_PushPopTest()
{
	RegisterLabel(TAG_StateMachine_Label_Test, FLabelSignature::CreateUObject(this, &ThisClass::Label_Test));
}

TCoroutine<> UMachineState_PushPopTest::Label_Default()
{
	if (GotoLabel(TAG_StateMachine_Label_Test))
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this]
		{
			OnMessageDelegate.Broadcast({ GetClass(), "Timer delay works", true });
		}, 1.5f, false);

		// Should be cancelled by the test label
		co_await Latent::Seconds(1.5);
		OnMessageDelegate.Broadcast({ GetClass(), "Latent execution has been cancelled", false });
	}
}

TCoroutine<> UMachineState_PushPopTest::Label_Test()
{
	// Cancel delayed fail report
	StopLatentExecution();

	if (LatentPushState)
	{
		co_await Latent::Seconds(1.0);
		co_await PushState(LatentPushState);
	}

	if (bLatentPopState)
	{
		co_await Latent::Seconds(1.0);
		PopState();
	}

	if (bNotifyTestFinish)
	{
		co_await Latent::Seconds(2.0);
		OnMessageDelegate.Broadcast({ GetClass(), "Test finished", true });
	}
}
