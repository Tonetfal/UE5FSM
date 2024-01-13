#include "MachineState_ExternalPushPopTest.h"

TCoroutine<> UMachineState_ExternalPushPopTest::Label_Default()
{
	RUN_LATENT_EXECUTION(Latent::Seconds, 0.5);
	BROADCAST_TEST_MESSAGE("Post default label", true);
}
