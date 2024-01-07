#include "MachineState_StartWithNotDefaultLabel.h"

#include "MachineState_PushPopTest.h"

UMachineState_StartWithNotDefaultLabel::UMachineState_StartWithNotDefaultLabel()
{
	RegisterLabel(TAG_StateMachine_Label_Test, FLabelSignature::CreateUObject(this, &ThisClass::Label_Test));
}

void UMachineState_StartWithNotDefaultLabel::Begin(TSubclassOf<UMachineState> PreviousState)
{
	Super::Begin(PreviousState);

	GOTO_LABEL(Test);
}

TCoroutine<> UMachineState_StartWithNotDefaultLabel::Label_Default()
{
	BROADCAST_TEST_MESSAGE("Ensure no entry", true);
	return Super::Label_Default();
}

TCoroutine<> UMachineState_StartWithNotDefaultLabel::Label_Test()
{
	BROADCAST_TEST_MESSAGE("Post test label", true);
	co_return;
}
