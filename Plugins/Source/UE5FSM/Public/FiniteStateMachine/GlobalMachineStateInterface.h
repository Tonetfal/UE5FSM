#pragma once

#include "UObject/Interface.h"

#include "GlobalMachineStateInterface.generated.h"

UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class UGlobalMachineStateInterface
	: public UInterface
{
	GENERATED_BODY()
};

/**
 * Global machine states are used as a supervisor in a finite state machine.
 *
 * Implement this interface to separate regular machine states from global ones to avoid mistakes.
 *
 * Right now it doesn't implement anything, but serves only as a thin distinction between normal and global states.
 */
class UE5FSM_API IGlobalMachineStateInterface
{
	GENERATED_BODY()

public:
	// Empty
};
