#pragma once

#include "CoreMinimal.h"

#include "FiniteStateMachineTestObject.generated.h"

class UFiniteStateMachine;

UCLASS(MinimalAPI, Hidden)
class AFiniteStateMachineTestActor
	: public AActor
{
	GENERATED_BODY()

public:
	AFiniteStateMachineTestActor();

public:
	UPROPERTY()
	TObjectPtr<UFiniteStateMachine> StateMachine = nullptr;
};
