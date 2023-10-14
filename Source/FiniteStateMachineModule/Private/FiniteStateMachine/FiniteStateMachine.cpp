#include "FiniteStateMachine/FiniteStateMachine.h"

#include "FiniteStateMachine/MachineState.h"
#include "FiniteStateMachine/MachineStateData.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LogFiniteStateMachine);

/**
 * Simple RAII wrapper for UFiniteStateMachineMutex::bModifyingInternalState.
 */
struct FFiniteStateMachineMutex
{
public:
	FFiniteStateMachineMutex(UFiniteStateMachine* Context)
		: bModifyingInternalState(Context->bModifyingInternalState)
	{
		ensure(!bModifyingInternalState);
		bModifyingInternalState = true;
	}

	~FFiniteStateMachineMutex()
	{
		ensure(bModifyingInternalState);
		bModifyingInternalState = false;
	}

private:
	bool& bModifyingInternalState;
};

UFiniteStateMachine::UFiniteStateMachine()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UFiniteStateMachine::Activate(bool bReset)
{
	Super::Activate(bReset);

	const bool bMyIsActive = IsActive();
	if (bMyIsActive && HasBeenInitialized() && !bActiveStatesBegan)
	{
		BeginActiveStates();
	}

	SetComponentTickEnabled(bMyIsActive);
}

void UFiniteStateMachine::InitializeComponent()
{
	if (GetWorld() && GetWorld()->IsPreviewWorld())
	{
		return;
	}

	// Dispatch all the states
	for (const TSubclassOf<UMachineState> State : InitialStateClassesToRegister)
	{
		RegisterState(State);
	}

	// Can remain nullptr
	if (IsValid(GlobalStateClass))
	{
		UMachineState* GlobalState = FindStateChecked(GlobalStateClass);
		ActiveGlobalState = CastChecked<UGlobalMachineState>(GlobalState);
	}

	// Can remain nullptr
	if (IsValid(InitialState))
	{
		ActiveState = FindStateChecked(InitialState);
		ActiveState->SetInitialLabel(InitialStateLabel);
	}

	Super::InitializeComponent();

	if (IsActive() && !bActiveStatesBegan)
	{
		BeginActiveStates();
	}
}

void UFiniteStateMachine::UninitializeComponent()
{
	if (ActiveGlobalState.IsValid())
	{
		ActiveGlobalState->OnStateAction(EStateAction::End, nullptr);
	}

	if (ActiveState.IsValid())
	{
		ActiveState->OnStateAction(EStateAction::End, nullptr);
	}

	Super::UninitializeComponent();
}

void UFiniteStateMachine::TickComponent(float DeltaTime, ELevelTick LevelTick,
	FActorComponentTickFunction* ActorComponentTickFunction)
{
	Super::TickComponent(DeltaTime, LevelTick, ActorComponentTickFunction);

	if (ActiveGlobalState.IsValid())
	{
		ActiveGlobalState->Tick(DeltaTime);
	}

	if (ActiveState.IsValid())
	{
		ActiveState->Tick(DeltaTime);
	}
}

bool UFiniteStateMachine::RegisterState(TSubclassOf<UMachineState> InStateClass)
{
	if (!IsValid(InStateClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Invalid state class."));
		return false;
	}

	if (InStateClass->ClassFlags & CLASS_Abstract)
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Machine state class is abstract."));
		return false;
	}

	if (IsStateRegistered(InStateClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("State class [%s] is already registered."), *InStateClass->GetName());
		return false;
	}

	RegisterState_Implementation(InStateClass);
	return true;
}

void UFiniteStateMachine::SetInitialState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label)
{
	if (IsValid(InitialState) || !ensure(!HasBeenInitialized()))
	{
		return;
	}

	if (!Label.IsValid())
	{
		// If label has not been specified, use the default one
		Label = TAG_StateMachine_Label_Default;
	}

	if (IsValid(InStateClass))
	{
		if (!InitialStateClassesToRegister.Contains(InStateClass))
		{
			UE_LOG(LogFiniteStateMachine, Warning, TEXT("State [%s] is not present in initial states to register list."),
				*InStateClass->GetName());
			return;
		}

		if (!UMachineState::IsLabelTagCorrect(Label))
		{
			UE_LOG(LogFiniteStateMachine, Warning, TEXT("Label [%s] is of wrong tag hierarchy."), *Label.ToString());
			return;
		}
	}

	InitialState = InStateClass;
	InitialStateLabel = Label;
}

void UFiniteStateMachine::SetGlobalState(TSubclassOf<UGlobalMachineState> InStateClass)
{
	if (IsValid(GlobalStateClass) || !ensure(!HasBeenInitialized()))
	{
		return;
	}

	if (IsValid(InStateClass))
	{
		if (!InitialStateClassesToRegister.Contains(InStateClass))
		{
			UE_LOG(LogFiniteStateMachine, Warning, TEXT("State [%s] is not present in initial states to register list."),
				*InStateClass->GetName());
			return;
		}
	}

	GlobalStateClass = InStateClass;
}

bool UFiniteStateMachine::GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label, bool bForceEvents)
{
	if (!ensure(HasBeenInitialized()))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to use GotoState before initialization."));
		return false;
	}

	if (bModifyingInternalState)
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to use GotoState while modifying internal state."));
		return false;
	}

	if (!IsValid(InStateClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Invalid state class."));
		return false;
	}

	// Disallow going to state when it's on the stack, but it's not the top-most one
	if (ActiveState.IsValid() && ActiveState->GetClass() != InStateClass && StatesStack.Contains(InStateClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to go to state [%s] as it's already present in the "
			"states stack."), *GetNameSafe(InStateClass));
		return false;
	}

	if (!IsStateRegistered(InStateClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("State [%s] is not registered in state machine."),
			*InStateClass->GetName());
		return false;
	}

	if (!Label.IsValid())
	{
		// If label has not been specified, use the default one
		Label = TAG_StateMachine_Label_Default;
	}

	if (!UMachineState::IsLabelTagCorrect(Label))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Label [%s] is of wrong tag hierarchy."), *Label.ToString());
		return false;
	}

	GotoState_Implementation(InStateClass, Label, bForceEvents);
	return true;
}

bool UFiniteStateMachine::GotoLabel(FGameplayTag Label)
{
	if (!ensure(HasBeenInitialized()))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to use GotoLabel before initialization."));
		return false;
	}

	if (!ActiveState.IsValid())
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("No state is active."));
		return false;
	}

	const bool bResult = !ActiveState->GotoLabel(Label);
	return bResult;
}

TCoroutine<bool> UFiniteStateMachine::PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label,
	bool* bOutPrematureResult)
{
	if (bOutPrematureResult)
	{
		*bOutPrematureResult = false;
	}

	if (!ensure(HasBeenInitialized()))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to use PushState before initialization."));
		co_return false;
	}

	if (bModifyingInternalState)
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to use PushState while modifying internal state."));
		co_return false;
	}

	if (!IsValid(InStateClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Invalid state class."));
		co_return false;
	}

	if (StatesStack.Contains(InStateClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to push state [%s] as it's already present in the "
			"stack."), *GetNameSafe(InStateClass));
		co_return false;
	}

	if (!IsStateRegistered(InStateClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("State [%s] is not registered in state machine."),
			*InStateClass->GetName());
		co_return false;
	}

	if (!Label.IsValid())
	{
		// If label has not been specified, use the default one
		Label = TAG_StateMachine_Label_Default;
	}

	if (!UMachineState::IsLabelTagCorrect(Label))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Label [%s] is of wrong tag hierarchy."), *Label.ToString());
		co_return false;
	}

	if (bOutPrematureResult)
	{
		// If PushState_Implementation is called, it means that we always will
		*bOutPrematureResult = true;
	}

	co_await PushState_Implementation(InStateClass, Label);
	co_return true;
}

bool UFiniteStateMachine::PopState()
{
	if (!ensure(HasBeenInitialized()))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to use PopState before initialization."));
		return false;
	}

	if (bModifyingInternalState)
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to use PushState while modifying internal state."));
		return false;
	}

	if (StatesStack.IsEmpty())
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Impossible to pop a state from stack as it's empty."));
		return false;
	}

	PopState_Implementation();
	return true;
}

bool UFiniteStateMachine::IsInState(TSubclassOf<UMachineState> InStateClass, bool bCheckStack) const
{
	if (!ActiveState.IsValid())
	{
		return false;
	}

	if (!bCheckStack)
	{
		const TSubclassOf<UMachineState> StateClass = ActiveState->GetClass();
		const bool bIsInState = StateClass == InStateClass;
		return bIsInState;
	}
	else
	{
		for (const TSubclassOf<UMachineState> StateClass : StatesStack)
		{
			if (StateClass == InStateClass)
			{
				return true;
			}
		}

		return false;
	}
}

bool UFiniteStateMachine::IsStateRegistered(TSubclassOf<UMachineState> InStateClass) const
{
	const UMachineState* FoundState = FindState(InStateClass);
	return !!FoundState;
}

UMachineStateData* UFiniteStateMachine::GetStateData(TSubclassOf<UMachineState> InStateClass,
	TSubclassOf<UMachineStateData> InStateDataClass) const
{
	if (!IsValid(InStateClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("Invalid state class."));
		return nullptr;
	}

	const UMachineState* FoundState = FindState(InStateClass);
	if (!IsValid(FoundState))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("State [%s] is not registered in state machine."),
			*InStateClass->GetName());
		return nullptr;
	}

	if (!IsValid(FoundState->BaseStateData))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("State [%s] lacks state data."),
			*InStateClass->GetName());
		return nullptr;
	}

	if (!FoundState->BaseStateData->IsA(InStateDataClass))
	{
		UE_LOG(LogFiniteStateMachine, Warning, TEXT("State [%s] data [%s] is not of class [%s]."),
			*InStateClass->GetName(), *FoundState->BaseStateData->GetName(), *InStateDataClass->GetName());
		return nullptr;
	}

	return FoundState->BaseStateData;
}

AActor* UFiniteStateMachine::GetAvatar() const
{
	AActor* Owner = GetOwner();
	if (const auto* Controller = Cast<AController>(Owner))
	{
		AActor* Avatar = Controller->GetPawn();
		return Avatar;
	}

	if (const auto* PlayerState = Cast<APlayerState>(Owner))
	{
		AActor* Avatar = PlayerState->GetPawn();
		return Avatar;
	}

	return Owner;
}

void UFiniteStateMachine::BeginActiveStates()
{
	ensure(!bActiveStatesBegan);

	if (ActiveGlobalState.IsValid())
	{
		ActiveGlobalState->OnStateAction(EStateAction::Begin, nullptr);
	}

	if (ActiveState.IsValid())
	{
		ActiveState->OnStateAction(EStateAction::Begin, nullptr);

		const TSubclassOf<UMachineState> StateClass = ActiveState->GetClass();
		StatesStack.Push(StateClass);
	}

	bActiveStatesBegan = true;
}

UMachineState* UFiniteStateMachine::RegisterState_Implementation(TSubclassOf<UMachineState> InStateClass)
{
	AActor* Owner = GetOwner();
	auto* State = NewObject<UMachineState>(Owner, InStateClass);
	check(IsValid(State));

	State->SetStateMachine(this);

	UE_LOG(LogFiniteStateMachine, VeryVerbose, TEXT("Machine state [%s] has been registered."), *State->GetName());

	RegisteredStates.Add(State);
	return State;
}

UMachineState* UFiniteStateMachine::FindState(TSubclassOf<UMachineState> InStateClass) const
{
	for (const TObjectPtr<UMachineState> State : RegisteredStates)
	{
		const TSubclassOf<UMachineState> StateClass = State->GetClass();
		if (StateClass == InStateClass)
		{
			return State;
		}
	}

	return nullptr;
}

UMachineState* UFiniteStateMachine::FindStateChecked(TSubclassOf<UMachineState> InStateClass) const
{
	UMachineState* FoundState = FindState(InStateClass);
	check(IsValid(FoundState));

	return FoundState;
}

void UFiniteStateMachine::GotoState_Implementation(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label,
	bool bForceEvents)
{
	// Disallow to change out state while we're running important code
	FFiniteStateMachineMutex(this);

	UMachineState* State = FindStateChecked(InStateClass);

	if (ActiveState.IsValid())
	{
		// Pop active state from the state stack without notifying the state, as we're not explicitely pushing/popping
		StatesStack.Pop();
	}

	if (ActiveState != State || bForceEvents)
	{
		TSubclassOf<UMachineState> PreviousStateClass = nullptr;
		if (ActiveState.IsValid())
		{
			PreviousStateClass = ActiveState->GetClass();
			ActiveState->OnStateAction(EStateAction::End, InStateClass);
		}

		ActiveState = State;
		ActiveState->OnStateAction(EStateAction::Begin, PreviousStateClass);
	}

	// Keep track of the stack without notifying the state, as we're not explicitely pushing/popping
	StatesStack.Push(InStateClass);

	// Tell the active state the requested label
	ActiveState->GotoLabel(Label);
}

TCoroutine<> UFiniteStateMachine::PushState_Implementation(TSubclassOf<UMachineState> InStateClass,
	FGameplayTag Label)
{
	TSubclassOf<UMachineState> PausedStateClass = nullptr;

	{
		// Disallow to change out state while we're running important code
		FFiniteStateMachineMutex(this);

		if (ActiveState.IsValid())
		{
			// The current active is paused while it's not the top-most
			PausedStateClass = ActiveState->GetClass();
			ActiveState->OnStateAction(EStateAction::Pause);
		}

		// Switch active state
		UMachineState* State = FindStateChecked(InStateClass);
		ActiveState = State;

		// Keep track of the stack; Notify new state about the action and requested label
		StatesStack.Push(InStateClass);
		ActiveState->OnStateAction(EStateAction::Push);
		ActiveState->GotoLabel(Label);
	}

	// Return code execution only after the paused state gets resumed
	co_await WaitUntilStateAction(PausedStateClass, EStateAction::Resume);
}

void UFiniteStateMachine::PopState_Implementation()
{
	// Disallow to change out state while we're running important code
	FFiniteStateMachineMutex(this);

	// Keep track of the stack
	StatesStack.Pop();
	ActiveState->OnStateAction(EStateAction::Pop);

	if (StatesStack.IsEmpty())
	{
		// Nothing to resume
		ActiveState = nullptr;
		return;
	}

	// Switch active state
	const TSubclassOf<UMachineState> StateClass = StatesStack.Top();
	UMachineState* State = FindStateChecked(StateClass);
	ActiveState = State;

	// Resume the paused state
	ActiveState->OnStateAction(EStateAction::Resume);
}

TCoroutine<> UFiniteStateMachine::WaitUntilStateAction(TSubclassOf<UMachineState> InStateClass,
	EStateAction StateAction) const
{
	if (!IsValid(InStateClass))
	{
		co_return;
	}

	// Wait until the requested state gets the requested state action
	auto OnReceived = FSimpleDelegate::CreateLambda([] { });
	UMachineState* State = FindStateChecked(InStateClass);
	const FDelegateHandle DelegateHandle = State->OnStateActionDelegate.AddLambda(
		[&OnReceived, StateAction] (EStateAction InStateAction)
	{
		if (InStateAction == StateAction)
		{
			// ReSharper disable once CppExpressionWithoutSideEffects
			OnReceived.Execute();
		}
	});

	co_await WhenAll(Latent::UntilDelegate(OnReceived));

	// Remove the callback from listeners
	State->OnStateActionDelegate.Remove(DelegateHandle);
}
