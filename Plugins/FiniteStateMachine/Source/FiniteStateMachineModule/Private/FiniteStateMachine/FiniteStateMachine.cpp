#include "FiniteStateMachine/FiniteStateMachine.h"

#include "FiniteStateMachine/FiniteStateMachineTypes.h"
#include "FiniteStateMachine/MachineState.h"
#include "FiniteStateMachine/MachineStateData.h"
#include "GameFramework/PlayerState.h"

#ifdef FSM_EXTEREME_VERBOSITY
	#define FSM_EXTEREME_VERBOSITY_STR FString::Printf(TEXT("Owner [%s]"), *GetNameSafe(GetOwner()))
#else
	#define FSM_EXTEREME_VERBOSITY_STR TEXT("")
#endif

#define FSM_LOG(VERBOSITY, MESSAGE, ...) UE_LOG(LogFiniteStateMachine, VERBOSITY, TEXT("%s - %s"), \
	*FSM_EXTEREME_VERBOSITY_STR, *FString::Printf(TEXT(MESSAGE), ## __VA_ARGS__))


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
	PrimaryComponentTick.bStartWithTickEnabled = false; // Depends on activation state

	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UFiniteStateMachine::Activate(bool bReset)
{
	Super::Activate(bReset);

	const bool bMyIsActive = IsActive();
	if (bMyIsActive)
	{
		GetWorld()->GetTimerManager().SetTimer(CancellersCleaningTimerHandle, this,
			&ThisClass::ClearStatesInvalidLatentExecutionCancellers, StateExecutionCancellersClearingInterval, true);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(CancellersCleaningTimerHandle);
	}

	if (bMyIsActive && bActiveStatesBegan)
	{
		if (ActiveGlobalState.IsValid())
		{
			ActiveGlobalState->OnStateAction(EStateAction::Resume);
		}

		if (ActiveState.IsValid())
		{
			ActiveGlobalState->OnStateAction(EStateAction::Resume);
		}
	}

	if (bMyIsActive && HasBeenInitialized() && !bActiveStatesBegan)
	{
		BeginActiveStates();
	}

	if (!bMyIsActive)
	{
		if (ActiveGlobalState.IsValid())
		{
			ActiveGlobalState->OnStateAction(EStateAction::Pause);
		}

		if (ActiveState.IsValid())
		{
			ActiveGlobalState->OnStateAction(EStateAction::Pause);
		}

		// If state machine was deactivated, we don't want anything to run
		StopEveryRunningLabels();
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
		ActiveGlobalState = GlobalState;
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
		ActiveGlobalState.Reset();
	}

	for (const TSubclassOf<UMachineState> Class : StatesStack)
	{
		UMachineState* State = FindStateChecked(Class);
		State->OnStateAction(EStateAction::End, nullptr);
	}

	StatesStack.Empty();

	// Sanity check
	StopEveryLatentExecution();
	StopEveryRunningLabels();

	for (const TObjectPtr<UMachineState> State : RegisteredStates)
	{
		State->bIsDestroyed = true;
		State->ConditionalBeginDestroy();
	}

	RegisteredStates.Empty();

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
		FSM_LOG(Warning, "Invalid state class.");
		return false;
	}

	if (InStateClass->ClassFlags & CLASS_Abstract)
	{
		FSM_LOG(Warning, "Machine state class is abstract.");
		return false;
	}

	if (IsStateRegistered(InStateClass))
	{
		FSM_LOG(Warning, "State class [%s] is already registered.", *InStateClass->GetName());
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
			FSM_LOG(Warning, "State [%s] is not present in initial states to register list.", *InStateClass->GetName());
			return;
		}

		if (!UMachineState::IsLabelTagCorrect(Label))
		{
			FSM_LOG(Warning, "Label [%s] is of wrong tag hierarchy.", *Label.ToString());
			return;
		}
	}

	InitialState = InStateClass;
	InitialStateLabel = Label;
}

void UFiniteStateMachine::SetGlobalState(TSubclassOf<UMachineState> InStateClass)
{
	if (IsValid(GlobalStateClass) || !ensure(!HasBeenInitialized()))
	{
		return;
	}

	if (ensure(GlobalStateClass->ImplementsInterface(UGlobalMachineStateInterface::StaticClass())))
	{
		FSM_LOG(Warning, "Global state [%s] must implement UGlobalMachineStateInterface.",
			*InStateClass->GetName());
		return;
	}

	if (IsValid(InStateClass))
	{
		if (!InitialStateClassesToRegister.Contains(InStateClass))
		{
			FSM_LOG(Warning, "State [%s] is not present in initial states to register list.", *InStateClass->GetName());
			return;
		}
	}

	GlobalStateClass = InStateClass;
}

bool UFiniteStateMachine::GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label, bool bForceEvents)
{
	if (!ensure(HasBeenInitialized()))
	{
		FSM_LOG(Warning, "Impossible to use GotoState before initialization.");
		return false;
	}

	if (bModifyingInternalState)
	{
		FSM_LOG(Warning, "Impossible to use GotoState while modifying internal state.");
		return false;
	}

	if (!IsValid(InStateClass))
	{
		FSM_LOG(Warning, "Invalid state class.");
		return false;
	}

	// Disallow going to state when it's on the stack, but it's not the top-most one
	if (ActiveState.IsValid() && ActiveState->GetClass() != InStateClass && StatesStack.Contains(InStateClass))
	{
		FSM_LOG(Warning, "Impossible to go to state [%s] as it's already present in the states stack.",
			*GetNameSafe(InStateClass));
		return false;
	}

	FString Reason;
	if (ActiveState.IsValid() && !ActiveState->CanSafelyDeactivate(OUT Reason))
	{
		FSM_LOG(Warning, "Impossible to go to state [%s] as active state [%s] is not safe from being deactivated. "
			"Reason: [%s]", *GetNameSafe(InStateClass), *ActiveState->GetName(), *Reason);
		return false;
	}

	if (!IsStateRegistered(InStateClass))
	{
		FSM_LOG(Warning, "State [%s] is not registered in state machine.", *InStateClass->GetName());
		return false;
	}

	if (!Label.IsValid())
	{
		// If label has not been specified, use the default one
		Label = TAG_StateMachine_Label_Default;
	}

	if (!UMachineState::IsLabelTagCorrect(Label))
	{
		FSM_LOG(Warning, "Label [%s] is of wrong tag hierarchy.", *Label.ToString());
		return false;
	}

	GotoState_Implementation(InStateClass, Label, bForceEvents);
	return true;
}

bool UFiniteStateMachine::GotoLabel(FGameplayTag Label)
{
	if (!ensure(HasBeenInitialized()))
	{
		FSM_LOG(Warning, "Impossible to use GotoLabel before initialization.");
		return false;
	}

	if (!ActiveState.IsValid())
	{
		FSM_LOG(Warning, "No state is active.");
		return false;
	}

	const bool bResult = !ActiveState->GotoLabel(Label);
	return bResult;
}

TCoroutine<> UFiniteStateMachine::PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label,
	bool* bOutPrematureResult)
{
	if (bOutPrematureResult)
	{
		*bOutPrematureResult = false;
	}

	if (!ensure(HasBeenInitialized()))
	{
		FSM_LOG(Warning, "Impossible to use PushState before initialization.");
		co_return;
	}

	if (bModifyingInternalState)
	{
		FSM_LOG(Warning, "Impossible to use PushState while modifying internal state.");
		co_return;
	}

	if (!IsValid(InStateClass))
	{
		FSM_LOG(Warning, "Invalid state class.");
		co_return;
	}

	if (StatesStack.ContainsByPredicate([InStateClass] (const TSubclassOf<UMachineState> StateClass)
		{
			return StateClass->IsChildOf(InStateClass);
		}))
	{
		FSM_LOG(Warning, "Impossible to push state [%s] as it's already present in the stack.",
			*GetNameSafe(InStateClass));
		co_return;
	}

	if (!IsStateRegistered(InStateClass))
	{
		FSM_LOG(Warning, "State [%s] is not registered in state machine.", *InStateClass->GetName());
		co_return;
	}

	if (!Label.IsValid())
	{
		// If label has not been specified, use the default one
		Label = TAG_StateMachine_Label_Default;
	}

	if (!UMachineState::IsLabelTagCorrect(Label))
	{
		FSM_LOG(Warning, "Label [%s] is of wrong tag hierarchy.", *Label.ToString());
		co_return;
	}

	if (bOutPrematureResult)
	{
		// If PushState_Implementation is called, it means that we always will
		*bOutPrematureResult = true;
	}

	co_await PushState_Implementation(InStateClass, Label);
}

bool UFiniteStateMachine::PopState()
{
	if (!ensure(HasBeenInitialized()))
	{
		FSM_LOG(Warning, "Impossible to use PopState before initialization.");
		return false;
	}

	if (bModifyingInternalState)
	{
		FSM_LOG(Warning, "Impossible to use PushState while modifying internal state.");
		return false;
	}

	if (StatesStack.IsEmpty())
	{
		FSM_LOG(Warning, "Impossible to pop a state from stack as it's empty.");
		return false;
	}

	PopState_Implementation();
	return true;
}

int32 UFiniteStateMachine::StopEveryLatentExecution()
{
	int32 StoppedLatentExecutios = 0;
	for (const TObjectPtr<UMachineState> State : RegisteredStates)
	{
		StoppedLatentExecutios += State->StopLatentExecution_Implementation();
	}

	if (StoppedLatentExecutios > 0)
	{
		FSM_LOG(VeryVerbose, "All [%d] latent executions have been cancelled.", StoppedLatentExecutios);
	}

	return StoppedLatentExecutios;
}

int32 UFiniteStateMachine::StopEveryRunningLabels()
{
	int32 StoppedLabels  = 0;
	for (const TObjectPtr<UMachineState> State : RegisteredStates)
	{
		StoppedLabels += State->StopRunningLabels();
	}

	if (StoppedLabels > 0)
	{
		FSM_LOG(VeryVerbose, "All [%d] running labels have been cancelled.", StoppedLabels);
	}

	return StoppedLabels;
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

TSubclassOf<UMachineState> UFiniteStateMachine::GetActiveStateClass() const
{
	if (!ActiveState.IsValid())
	{
		return nullptr;
	}

	const TSubclassOf<UMachineState> ActiveStateClass = ActiveState->GetClass();
	return ActiveStateClass;
}

bool UFiniteStateMachine::IsStateRegistered(TSubclassOf<UMachineState> InStateClass) const
{
	const UMachineState* FoundState = FindState(InStateClass);
	return !!FoundState;
}

UMachineState* UFiniteStateMachine::GetState(TSubclassOf<UMachineState> InStateClass) const
{
	if (!IsValid(InStateClass))
	{
		FSM_LOG(Warning, "Invalid state class.");
		return nullptr;
	}

	UMachineState* FoundState = FindState(InStateClass);
	if (!IsValid(FoundState))
	{
		FSM_LOG(Warning, "State [%s] is not registered.", *InStateClass->GetName());
		return nullptr;
	}

	if (!FoundState->IsA(InStateClass))
	{
		FSM_LOG(Warning, "State [%s] is not of class [%s].", *InStateClass->GetName(), *InStateClass->GetName());
		return nullptr;
	}

	return FoundState;
}

UMachineStateData* UFiniteStateMachine::GetStateData(TSubclassOf<UMachineState> InStateClass,
	TSubclassOf<UMachineStateData> InStateDataClass) const
{
	if (!IsValid(InStateClass))
	{
		FSM_LOG(Warning, "Invalid state class.");
		return nullptr;
	}

	if (!IsValid(InStateDataClass))
	{
		FSM_LOG(Warning, "Invalid state data class.");
		return nullptr;
	}

	const UMachineState* FoundState = FindState(InStateClass);
	if (!IsValid(FoundState))
	{
		FSM_LOG(Warning, "State [%s] is not registered in state machine.", *InStateClass->GetName());
		return nullptr;
	}

	if (!IsValid(FoundState->BaseStateData))
	{
		FSM_LOG(Warning, "State [%s] lacks state data.", *InStateClass->GetName());
		return nullptr;
	}

	if (!FoundState->BaseStateData->IsA(InStateDataClass))
	{
		FSM_LOG(Warning, "State [%s] data [%s] is not of class [%s].",
			*InStateClass->GetName(), *FoundState->BaseStateData->GetName(), *InStateDataClass->GetName());
		return nullptr;
	}

	return FoundState->BaseStateData;
}

const TArray<TSubclassOf<UMachineState>>& UFiniteStateMachine::GetStatesStack() const
{
	return StatesStack;
}

TArray<TSubclassOf<UMachineState>> UFiniteStateMachine::GetRegisteredStateClasses() const
{
	TArray<TSubclassOf<UMachineState>> RegisteredStateClasses;
	for (const TObjectPtr<UMachineState> RegisteredState : RegisteredStates)
	{
		RegisteredStateClasses.Add(RegisteredState.GetClass());
	}

	return RegisteredStateClasses;
}

TSubclassOf<UMachineState> UFiniteStateMachine::GetGlobalStateClass() const
{
	return GlobalStateClass;
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

	FSM_LOG(VeryVerbose, "Machine state [%s] has been registered.", *State->GetName());

	RegisteredStates.Add(State);
	return State;
}

UMachineState* UFiniteStateMachine::FindState(TSubclassOf<UMachineState> InStateClass) const
{
	for (const TObjectPtr<UMachineState> State : RegisteredStates)
	{
		const TSubclassOf<UMachineState> StateClass = State->GetClass();
		if (StateClass->IsChildOf(InStateClass))
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

	co_await Latent::UntilDelegate(OnReceived);
	co_await FinishNowIfCanceled();

	// Remove the callback from listeners
	State->OnStateActionDelegate.Remove(DelegateHandle);
}

void UFiniteStateMachine::ClearStatesInvalidLatentExecutionCancellers()
{
	int32 RemovedCancellers = 0;
	for (const TObjectPtr<UMachineState> State : RegisteredStates)
	{
		RemovedCancellers += State->ClearInvalidLatentExecutionCancellers();
	}

	if (RemovedCancellers > 0)
	{
		FSM_LOG(VeryVerbose, "All [%d] running invalid latent execution cancellers have been cancelled.",
			RemovedCancellers);
	}
}
