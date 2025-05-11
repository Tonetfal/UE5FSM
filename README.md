# UE5FSM

This is a Finite State Machine (FSM) developed for Unreal Engine 5 that mimics the behavior of Unreal Engine's 3 FSM. 
The tool is developed with a focus on gameplay logic to facilitate the development of complex behavior.

## Installation

Download the release that you wish to use from the [Release](https://github.com/Tonetfal/UE5FSM/releases) page, and copy 
its contents into your project's Plugins directory.

The plugin is dependent on [UE5Coro](https://github.com/landelare/ue5coro). Refer to its documentation for the 
installation process and other features. This plugin supports UE5Coro 2, and was tested on 2.1. It should be 
compatible with UE5Coro 1.10, at least for 1.4.0-alpha release.

## Description

Finite State Machine is an actor component meant to be attached to actors that have a state. You can make 
different logic using that. Each piece of logic has to be built using [machine states](Docs/States.md) and their 
[labels](Docs/Labels.md). The machine states have a wide range of events and functions to control the logic flow. The 
machine state always knows what state it's to start or abort context-dependent logic. Every machine state 
can have its unique [data](Docs/StateData.md) object to store its own project-specific data in. There are 
[tools](Docs/Debug.md) to debug your own finite state machines easily. If those aren't enough, you can always extend 
them pretty easily by adding more debug information to it.

Read more about the plugin in the [documentation](Docs) and in the source code, as it's all well documented.

## Example

Take a look at how you can make a simple AI that can walk to a target and attack it.

```c++
using namespace UE5Coro;

UCLASS()
class AMyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMyAIController();

protected:
	//~AAIController Interface
	virtual void OnPossess(APawn* InPawn) override;
	//~End of AAIController Interface

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="AI")
	TObjectPtr<UFiniteStateMachine> StateMachine = nullptr;
};

UCLASS()
class USeekingState : public UMachineState
{
	GENERATED_BODY()

protected:
	//~UMachineState Interface
	virtual TCoroutine<> Label_Default() override;
	//~End of UMachineState Interface

	AActor* GetTarget() const;
};

UCLASS()
class UAttackingState : public UMachineState
{
	GENERATED_BODY()

protected:
	//~UMachineState Interface
	virtual TCoroutine<> Label_Default() override;
	//~End of UMachineState Interface

	void StartAttacking();
	UAnimMontage* GetAttackAnimation();
};
```

```c++
AMyAIController::AMyAIController()
{
	StateMachine = CreateDefaultSubobject<UFiniteStateMachine>("FiniteStateMachine");
	StateMachine->bAutoActivate = false;

	// It's better to register them within blueprint properties; we're doing that here to show the shortest variant
	StateMachine->RegisterState(USeekingState::StaticClass());
	StateMachine->RegisterState(UAttackingState::StaticClass());
}

void AMyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Activate it once the controller gets a pawn to make the FSM to run logic upon it
	StateMachine->Activate(true);
}

TCoroutine<> USeekingState::Label_Default()
{
	// Walk to a target
	RUN_LATENT_EXECUTION(UE5Coro::UE5FSM::AI::AIMoveTo, GetOwner<AAIController>(), GetTarget());
	
	// Upon reaching the target push the attacking state to harm it
	PUSH_STATE(UAttackingState);
	
	// Repeat the function
	GOTO_LABEL(Default);
}

AActor* USeekingState::GetTarget() const
{
	// Somehow get a target...
}

TCoroutine<> UAttackingState::Label_Default()
{
	StartAttacking();

	// Wait until the attack animation ends
	const auto* Character = GetOwner<AAIController>()->GetPawn<ACharacter>();
	const UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	while (AnimInstance->Montage_IsPlaying(GetAttackAnimation()))
	{
		// Wait until the animation doesn't end
		RUN_LATENT_EXECUTION(Latent::NextTick);
	}

	// Finish attacking; this will return us to the last state, in this case to the seeking state
	POP_STATE();
}

void UAttackingState::StartAttacking()
{
	// Somehow start attacking...
}

UAnimMontage* UAttackingState::GetAttackAnimation()
{
	// Somehow get the animation...
}
```