#include "FiniteStateMachine/FiniteStateMachine.h"

#include "FiniteStateMachine/FiniteStateMachineLog.h"
#include "FiniteStateMachine/MachineState.h"
#include "FiniteStateMachine/MachineStateData.h"
#include "GameFramework/PlayerState.h"

using namespace UE5Coro;

void FFSM_PushRequestHandle::BindOnResultCallback(const FOnPendingPushRequestSignature::FDelegate&& Callback) const
{
	if (StateMachine.IsValid())
	{
		StateMachine->GetOnPendingPushRequestResultDelegate(*this).Add(Callback);
	}
}

void FFSM_PushRequestHandle::Cancel() const
{
	if (StateMachine.IsValid())
	{
		StateMachine->CancelPushRequest(*this);
	}
}

bool FFSM_PushRequestHandle::IsPending() const
{
	if (StateMachine.IsValid())
	{
		return StateMachine->IsPushRequestPending(*this);
	}

	return false;
}

UFiniteStateMachine::UFiniteStateMachine()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // Depends on activation state

	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UFiniteStateMachine::PostReinitProperties()
{
	Super::PostReinitProperties();

	for (const TSubclassOf<UMachineState> StateClass : InitialStateClassesToRegister)
	{
		if (IsValid(StateClass))
		{
			ensureMsgf(!StateClass->ImplementsInterface(UGlobalMachineStateInterface::StaticClass()),
				TEXT("InitialStateClassesToRegister container contains a global machine state [%s]. "
					"Remove it out of the array as global machine state will be automatically registered if it's "
					"assigned to the GlobalStateClass"), *StateClass->GetName());
		}
	}
}

void UFiniteStateMachine::Activate(bool bReset)
{
	Super::Activate(bReset);

	const bool bMyIsActive = IsActive();
	if (bMyIsActive)
	{
		GetTimerManager().SetTimer(CancellersCleaningTimerHandle, this,
			&ThisClass::ClearStatesInvalidLatentExecutionCancellers, StateExecutionCancellersClearingInterval, true);
	}
	else
	{
		GetTimerManager().ClearTimer(CancellersCleaningTimerHandle);
	}

	if (bMyIsActive && bActiveStatesBegan)
	{
		if (ActiveGlobalState.IsValid())
		{
			ActiveGlobalState->OnStateAction(EStateAction::Resume, nullptr);
		}

		if (ActiveState.IsValid())
		{
			ActiveGlobalState->OnStateAction(EStateAction::Resume, nullptr);
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
			ActiveGlobalState->OnStateAction(EStateAction::Pause, nullptr);
		}

		if (ActiveState.IsValid())
		{
			ActiveGlobalState->OnStateAction(EStateAction::Pause, nullptr);
		}

		// If state machine was deactivated, we don't want anything to run
		StopEveryRunningLabel();
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
	for (const TSubclassOf<UMachineState> StateClass : InitialStateClassesToRegister)
	{
		if (IsValid(StateClass))
		{
			ensureMsgf(!StateClass->ImplementsInterface(UGlobalMachineStateInterface::StaticClass()),
				TEXT("InitialStateClassesToRegister container contains a global machine state [%s]. "
					"Remove it out of the array as global machine state will be automatically registered if it's "
					"assigned to the GlobalStateClass"), *StateClass->GetName());
		}

		RegisterState(StateClass);
	}

	// Can remain nullptr
	if (IsValid(GlobalStateClass))
	{
		// Automatically register the global state
		RegisterState(GlobalStateClass);

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

	// Finilize the stack
	ClearStack();

	// Sanity check
	StopEveryLatentExecution();
	StopEveryRunningLabel();

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
		FSM_LOG(Warning, "Machine state class [%s] is abstract.", *InStateClass->GetName());
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
		FSM_LOG(Warning, "Impossible to use GotoState on [%s] before initialization.", *GetNameSafe(InStateClass));
		return false;
	}

	if (!IsValid(InStateClass))
	{
		FSM_LOG(Warning, "Invalid state class.");
		return false;
	}

	// Disallow going to state when it's on the stack, but it's not the top-most one
	if (IsInState(InStateClass, true) && ActiveState->GetClass() != InStateClass)
	{
		FSM_LOG(Warning, "Impossible to go to state [%s] as it's already present on the states stack.",
			*GetNameSafe(InStateClass));
		return false;
	}

	if (IsTransitionBlockedTo(InStateClass))
	{
		FSM_LOG(Warning, "Impossible to go to state [%s] as active state [%s] has disallowed this particular "
			"transition.", *GetNameSafe(InStateClass), *ActiveState->GetName());
		return false;
	}

	FString Reason;
	if (!CanActiveStateSafelyDeactivate(Reason))
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

	if (!UMachineState::IsLabelTagCorrect(Label))
	{
		FSM_LOG(Warning, "Label [%s] is of wrong tag hierarchy.", *Label.ToString());
		return false;
	}

	if (bIsRunningLatentRequest)
	{
		FSM_LOG(Warning, "A latent request is already running. Avoid calling multiple of them at once.");
		return false;
	}

	if (!IsActiveStateDispatchingEvent())
	{
		GotoState_Implementation(InStateClass, Label, bForceEvents);
	}
	else
	{
		GotoState_LatentImplementation(InStateClass, Label, bForceEvents);
	}

	return true;
}

bool UFiniteStateMachine::EndState()
{
	if (!ensure(HasBeenInitialized()))
	{
		FSM_LOG(Warning, "Impossible to end a state before initialization.");
		return false;
	}

	if (!ActiveState.IsValid())
	{
		FSM_LOG(Warning, "Impossible to end a state as there's nothing to end.");
		return false;
	}

	if (bIsRunningLatentRequest)
	{
		FSM_LOG(Warning, "A latent request is already running. Avoid calling multiple of them at once.");
		return false;
	}

	if (!IsActiveStateDispatchingEvent())
	{
		EndState_Implementation();
	}
	else
	{
		EndState_LatentImplementation();
	}

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
		FSM_LOG(Warning, "Impossible to push state [%s] before initialization.", *GetNameSafe(InStateClass));
		co_return;
	}

	if (!IsValid(InStateClass))
	{
		FSM_LOG(Warning, "Invalid state class.");
		co_return;
	}

	if (IsInState(InStateClass, true))
	{
		FSM_LOG(Warning, "Impossible to push state [%s] as it's already present on the states stack.",
			*GetNameSafe(InStateClass));
		co_return;
	}

	if (!IsStateRegistered(InStateClass))
	{
		FSM_LOG(Warning, "State [%s] is not registered in state machine.", *InStateClass->GetName());
		co_return;
	}

	if (!UMachineState::IsLabelTagCorrect(Label))
	{
		FSM_LOG(Warning, "Label [%s] is of wrong tag hierarchy.", *Label.ToString());
		co_return;
	}

	if (bIsRunningLatentRequest)
	{
		FSM_LOG(Warning, "A latent request is already running. Avoid calling multiple of them at once.");
		co_return;
	}

	if (bOutPrematureResult)
	{
		*bOutPrematureResult = true;
	}

	if (!IsActiveStateDispatchingEvent())
	{
		PushState_Implementation(InStateClass, Label);
	}
	else
	{
		PushState_LatentImplementation(InStateClass, Label);
	}
}

TCoroutine<> UFiniteStateMachine::PushStateQueued(FFSM_PushRequestHandle& OutHandle,
	TSubclassOf<UMachineState> InStateClass, FGameplayTag Label)
{
	if (!IsValid(InStateClass))
	{
		FSM_LOG(Warning, "Invalid state class.");
		co_return;
	}

	if (!UMachineState::IsLabelTagCorrect(Label))
	{
		FSM_LOG(Warning, "Label [%s] is of wrong tag hierarchy.", *Label.ToString());
		co_return;
	}

	if (!HasBeenInitialized())
	{
		FSM_LOG(Log, "Impossible to immediately push state [%s] before initialization. "
			"The operation will be queued.", *GetNameSafe(InStateClass));

		co_await AddAndWaitPendingPushRequest(OutHandle, InStateClass, Label);
		co_return;
	}

	if (IsTransitionBlockedTo(InStateClass))
	{
		FSM_LOG(Log, "Impossible to immediately go to state [%s] as active state [%s] has disallowed this "
			"particular transition. The operation will be queued.",
			*GetNameSafe(InStateClass), *ActiveState->GetName());

		co_await AddAndWaitPendingPushRequest(OutHandle, InStateClass, Label);
		co_return;
	}

	if (IsInState(InStateClass, true))
	{
		FSM_LOG(Log, "Impossible to immediately push state [%s] as it's already present on the states stack. "
			"The operation will be queued.", *GetNameSafe(InStateClass));

		co_await AddAndWaitPendingPushRequest(OutHandle, InStateClass, Label);
		co_return;
	}

	if (!IsStateRegistered(InStateClass))
	{
		FSM_LOG(Log, "State [%s] is not registered in state machine. The operation will be queued.",
			*InStateClass->GetName());

		co_await AddAndWaitPendingPushRequest(OutHandle, InStateClass, Label);
		co_return;
	}

	if (bIsRunningLatentRequest)
	{
		FSM_LOG(Log, "A latent request is already running. Avoid calling multiple of them at once. "
			"The operation will be queued.");

		co_await AddAndWaitPendingPushRequest(OutHandle, InStateClass, Label);
		co_return;
	}

	PushState_Implementation(InStateClass, Label);
}

bool UFiniteStateMachine::PopState()
{
	if (!ensure(HasBeenInitialized()))
	{
		FSM_LOG(Warning, "Impossible to pop a state before initialization.");
		return false;
	}

	if (StatesStack.IsEmpty())
	{
		FSM_LOG(Warning, "Impossible to pop a state from states stack as it's empty.");
		return false;
	}

	if (bIsRunningLatentRequest)
	{
		FSM_LOG(Warning, "A latent request is already running. Avoid calling multiple of them at once.");
		return false;
	}

	if (!IsActiveStateDispatchingEvent())
	{
		PopState_Implementation();
	}
	else
	{
		PopState_LatentImplementation();
	}

	return true;
}

TCoroutine<> UFiniteStateMachine::AddAndWaitPendingPushRequest(FFSM_PushRequestHandle& OutHandle,
	TSubclassOf<UMachineState> InStateClass, FGameplayTag Label)
{
	FFSM_PushRequestHandle Handle;
	Handle.StateMachine = this;
	Handle.ID = FFSM_PushRequestHandle::s_ID;
	Handle.s_ID++;

	OutHandle = Handle;

	// Push a request to the queue
	FPendingPushRequest& Request = PendingPushRequests.AddDefaulted_GetRef();
	Request.StateClass = InStateClass;
	Request.Label = Label;
	Request.ID = Handle.ID;

	FSM_LOG(VeryVerbose, "Add pending push request. ID [%d] State [%s] Label [%s]",
		Handle.ID, *InStateClass->GetName(), *Label.ToString())

	// Wait until the request is not handled
	while (true)
	{
		// Listen for OnPush events
		const auto [RequestID, Result] = co_await OnPushRequestResultDelegate;

		// Wait for our ID
		if (RequestID == Handle.ID)
		{
			co_return;
		}
	}
}

void UFiniteStateMachine::UpdatePushQueue()
{
	if (!PendingPushRequests.IsEmpty())
	{
		const FPendingPushRequest Request = PendingPushRequests[0];
		PushState_Pending(Request);
	}
}

void UFiniteStateMachine::PushState_Pending(FPendingPushRequest Request)
{
	if (!HasBeenInitialized())
	{
		return;
	}

	if (IsInState(Request.StateClass))
	{
		return;
	}

	if (IsTransitionBlockedTo(Request.StateClass))
	{
		return;
	}

	if (!IsStateRegistered(Request.StateClass))
	{
		return;
	}

	if (bIsRunningLatentRequest)
	{
		return;
	}

	// Sanity check; Should never happen
	if (ActiveState.IsValid())
	{
		ensure(!ActiveState->IsDispatchingEvent());
	}

	FSM_LOG(VeryVerbose, "Execute pending push request. ID [%d] State [%s] Label [%s]",
		Request.ID, *Request.StateClass->GetName(), *Request.Label.ToString())

	// Remove the request from the queue as it's about to get pushed
	PendingPushRequests.RemoveSingle(FPendingPushRequest(Request.ID));

	{
		// Notify about the success just before executing the pending request
		const FOnPendingPushRequestSignature* FoundDelegate =
			OnPendingPushRequestResultDelegates.Find(Request.ID);
		if (FoundDelegate)
		{
			FoundDelegate->Broadcast(EFSM_PendingPushRequestResult::Success);

			// Remove the delegates as this one will never be fired anymore
			OnPendingPushRequestResultDelegates.Remove(Request.ID);
		}

		// Actually apply the request
		PushState_Implementation(Request.StateClass, Request.Label);
	}

	// Notify about the action
	OnPushRequestResultDelegate.Broadcast(Request.ID, EPushRequestResult::Success);
}

int32 UFiniteStateMachine::ClearStack()
{
	if (IsActiveStateDispatchingEvent())
	{
		FSM_LOG(Warning, "The active state [%s] is dispatching an event. It's impossible to clear the stack.",
			*ActiveState->GetName());
		return 0;
	}

	int32 StatesPopped = 0;
	while (!StatesStack.IsEmpty() && EndState())
	{
		StatesPopped++;
	}

	return StatesPopped;
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

int32 UFiniteStateMachine::StopEveryRunningLabel()
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

bool UFiniteStateMachine::CancelPushRequest(FFSM_PushRequestHandle Handle)
{
	const int32 FoundIndex = PendingPushRequests.Find(FPendingPushRequest(Handle.ID));
	if (FoundIndex == INDEX_NONE)
	{
		return false;
	}

	const FPendingPushRequest PushRequest = PendingPushRequests[FoundIndex];
	PendingPushRequests.RemoveAt(FoundIndex);

	FSM_LOG(VeryVerbose, "Cancel pending push request. ID [%d] State [%s] Label [%s]",
		PushRequest.ID, *PushRequest.StateClass->GetName(), *PushRequest.Label.ToString())

	const FOnPendingPushRequestSignature* FoundDelegate = OnPendingPushRequestResultDelegates.Find(Handle.ID);
	if (FoundDelegate)
	{
		FoundDelegate->Broadcast(EFSM_PendingPushRequestResult::Canceled);

		// Remove the delegates as this one will never be fired anymore
		OnPendingPushRequestResultDelegates.Remove(Handle.ID);
	}

	OnPushRequestResultDelegate.Broadcast(Handle.ID, EPushRequestResult::Canceled);
	return true;
}

bool UFiniteStateMachine::IsPushRequestPending(FFSM_PushRequestHandle Handle) const
{
	const bool bContains = PendingPushRequests.Contains(FPendingPushRequest(Handle.ID));
	return bContains;
}

FOnPendingPushRequestSignature& UFiniteStateMachine::GetOnPendingPushRequestResultDelegate(
	FFSM_PushRequestHandle Handle)
{
	ensureMsgf(IsPushRequestPending(Handle), TEXT("Push request handle that does not identify any active pending "
		"request has been passed. The returned delegate will never be fired."));

	return OnPendingPushRequestResultDelegates.FindOrAdd(Handle.ID);
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

TSubclassOf<UMachineState> UFiniteStateMachine::GetInitialMachineState() const
{
	return InitialState;
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

FTimerManager& UFiniteStateMachine::GetTimerManager() const
{
	const UWorld* World = GetWorld();
	check(IsValid(World));

	return World->GetTimerManager();
}

#ifdef WITH_EDITOR
TArray<UFiniteStateMachine::FDebugStateAction> UFiniteStateMachine::GetLastStateActionsStack() const
{
	TArray<FDebugStateAction> ReturnValue;

	auto Deque = LastStateActionsStack._Get_container();
	for (auto It = Deque.rbegin(); It != Deque.rend(); ++It)
	{
		ReturnValue.Add(*It);
	}

	return ReturnValue;
}
#endif

void UFiniteStateMachine::OnStateAction(UMachineState* State, EStateAction StateAction)
{
#ifdef WITH_EDITOR
	FDebugStateAction Action;
	Action.State = State;
	Action.Action = StateAction;
	Action.ActionTime = GetWorld()->GetTimeSeconds();

	// Save the action for debug purposes
	LastStateActionsStack.push(Action);

	// Don't store too many entries; a hundred should be more than enough
	constexpr int32 MaxSize = 100;
	while (LastStateActionsStack.size() > MaxSize)
	{
		LastStateActionsStack.pop();
	}
#endif

	// Anytime the stack is changed, update the queue so that any pending request is dispatched
	UpdatePushQueue();
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
		const TSubclassOf<UMachineState> StateClass = ActiveState->GetClass();
		StatesStack.Push(StateClass);

		ActiveState->OnStateAction(EStateAction::Begin, nullptr);
	}

	bActiveStatesBegan = true;
}

UMachineState* UFiniteStateMachine::RegisterState_Implementation(TSubclassOf<UMachineState> InStateClass)
{
	AActor* Owner = GetOwner();
	auto* State = NewObject<UMachineState>(Owner, InStateClass);
	check(IsValid(State));

	State->OnStateActionDelegate.AddUObject(this, &ThisClass::OnStateAction);
	State->SetStateMachine(this);

	FSM_LOG(Log, "Machine state [%s] has been registered.", *State->GetName());

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
	UMachineState* State = FindStateChecked(InStateClass);

	if (ActiveState.IsValid())
	{
		// Pop active state from the state stack without notifying the state, as we're not explicitly pushing/popping
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

		// Keep track of the stack without notifying the state, as we're not explicitely pushing/popping
		StatesStack.Push(InStateClass);

		// Tell the active state the requested label
		ActiveState->GotoLabel(Label);

		// Tell the state what's happening to it. Note: When forcing events,
		// UMachineState::OnAddedToStack will be fired even thogh the state is already on the stack
		ActiveState->OnStateAction(EStateAction::Begin, PreviousStateClass);
	}
	else
	{
		// Keep track of the stack without notifying the state, as we're not explicitely pushing/popping
		StatesStack.Push(InStateClass);
	}
}

TCoroutine<> UFiniteStateMachine::GotoState_LatentImplementation(TSubclassOf<UMachineState> InStateClass,
	FGameplayTag Label, bool bForceEvents)
{
	co_await WaitUntilActiveStateEventDispatch();
	GotoState_Implementation(InStateClass, Label, bForceEvents);
	bIsRunningLatentRequest = false;
}

TCoroutine<> UFiniteStateMachine::PushState_Implementation(TSubclassOf<UMachineState> InStateClass,
	FGameplayTag Label)
{
	TSubclassOf<UMachineState> PausedStateClass = nullptr;

	{
		if (ActiveState.IsValid())
		{
			// The current active is paused while it's not the top-most
			PausedStateClass = ActiveState->GetClass();
			ActiveState->OnStateAction(EStateAction::Pause, InStateClass);
		}

		// Switch active state
		UMachineState* State = FindStateChecked(InStateClass);
		ActiveState = State;

		// Keep track of the stack; Notify new state about the action and requested label
		StatesStack.Push(InStateClass);

		// Tell the active state the requested label
		ActiveState->GotoLabel(Label);

		// Tell the state what's happening to it
		ActiveState->OnStateAction(EStateAction::Push, PausedStateClass);
	}

	if (ActiveState.IsValid())
	{
		/**
		 * OnPushed of the state that was asked to push might've happened something that altered the active state,
		 * resulting it into becoming the one that we've just paused. In other words, the pushed state could pop itself
		 * in OnPushed.
		 */
		const TSubclassOf<UMachineState> ActiveStateClass = ActiveState->GetClass();
		if (ActiveStateClass == PausedStateClass)
		{
			co_return;
		}
	}

	// Return code execution only after the paused state gets resumed
	co_await WaitUntilStateAction(PausedStateClass, EStateAction::Resume);
}

TCoroutine<> UFiniteStateMachine::PushState_LatentImplementation(TSubclassOf<UMachineState> InStateClass,
	FGameplayTag Label)
{
	co_await WaitUntilActiveStateEventDispatch();
	co_await PushState_Implementation(InStateClass, Label);
	bIsRunningLatentRequest = false;
}

void UFiniteStateMachine::PopState_Implementation()
{
	TSubclassOf<UMachineState> ResumedState = nullptr;
	const int32 Num = StatesStack.Num();
	if (Num > 1)
	{
		ResumedState = StatesStack.Last(1);
	}

	// Keep track of the stack
	StatesStack.Pop();
	const TSubclassOf<UMachineState> PoppedState = ActiveState->GetClass();
	ActiveState->OnStateAction(EStateAction::Pop, ResumedState);

	if (!ResumedState)
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
	ActiveState->OnStateAction(EStateAction::Resume, PoppedState);
}

TCoroutine<> UFiniteStateMachine::PopState_LatentImplementation()
{
	co_await WaitUntilActiveStateEventDispatch();
	PopState_Implementation();
	bIsRunningLatentRequest = false;
}

void UFiniteStateMachine::EndState_Implementation()
{
	TSubclassOf<UMachineState> ResumedState = nullptr;
	const int32 Num = StatesStack.Num();
	if (Num > 1)
	{
		ResumedState = StatesStack.Last(1);
	}

	// Keep track of the stack
	StatesStack.Pop();
	const TSubclassOf<UMachineState> EndedState = ActiveState->GetClass();
	ActiveState->OnStateAction(EStateAction::End, ResumedState);

	if (!ResumedState)
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
	ActiveState->OnStateAction(EStateAction::Resume, EndedState);
}

TCoroutine<> UFiniteStateMachine::EndState_LatentImplementation()
{
	co_await WaitUntilActiveStateEventDispatch();
	EndState_Implementation();
	bIsRunningLatentRequest = false;
}

TCoroutine<> UFiniteStateMachine::WaitUntilStateAction(TSubclassOf<UMachineState> InStateClass,
	EStateAction StateAction) const
{
	if (!IsValid(InStateClass))
	{
		co_return;
	}

	// Wait until the requested state gets the requested state action
	UMachineState* State = FindStateChecked(InStateClass);
	while (true)
	{
		const auto [InState, InAction] = co_await State->OnStateActionDelegate;
		if (InAction == StateAction)
		{
			break;
		}
	}

	co_await FinishNowIfCanceled();
}

TCoroutine<> UFiniteStateMachine::WaitUntilActiveStateEventDispatch()
{
	ensure(!bIsRunningLatentRequest);
	check(ActiveState.IsValid());

	ensure(ActiveState->IsDispatchingEvent());
	bIsRunningLatentRequest = true;

	co_await ActiveState->OnFinishedDispatchingEvent;

	ensure(!ActiveState->IsDispatchingEvent());
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

bool UFiniteStateMachine::IsTransitionBlockedTo(TSubclassOf<UMachineState> InStateClass) const
{
	if (!ActiveState.IsValid())
	{
		return false;
	}

	const bool bIsBlocking = ActiveState->StatesBlocklist.ContainsByPredicate(
		[InStateClass] (const TSubclassOf<UMachineState> Item)
 		{
 			return InStateClass->IsChildOf(Item);
 		});

	return bIsBlocking;
}

bool UFiniteStateMachine::CanActiveStateSafelyDeactivate(FString& OutReason) const
{
	OutReason = "";

	if (!ActiveState.IsValid())
	{
		return true;
	}

	return ActiveState->CanSafelyDeactivate(OUT OutReason);
}

bool UFiniteStateMachine::IsActiveStateDispatchingEvent() const
{
	return ActiveState.IsValid() && ActiveState->IsDispatchingEvent();
}
