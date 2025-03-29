#pragma once

#include "UObject/Object.h"

#include "MachineStateData.generated.h"

/**
 * Object used to store persistant data of a state that should be accessible from other states to read and write to pass
 * the information around.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=("Finite State Machine"))
class UE5FSM_API UMachineStateData
	: public UObject
{
	GENERATED_BODY()

public:
	// Empty
};
