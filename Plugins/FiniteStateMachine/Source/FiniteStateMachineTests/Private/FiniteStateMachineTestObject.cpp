#include "FiniteStateMachineTestObject.h"

#include "FiniteStateMachine/FiniteStateMachine.h"

AFiniteStateMachineTestActor::AFiniteStateMachineTestActor()
{
	StateMachine = CreateDefaultSubobject<UFiniteStateMachine>("FiniteStateMachine");
}
