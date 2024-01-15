#include "FiniteStateMachine/MachineState.h"

#include "FiniteStateMachine/FiniteStateMachine.h"
#include "FiniteStateMachine/FiniteStateMachineLog.h"
#include "FiniteStateMachine/MachineStateData.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateMachine_Label, "StateMachine.Label");
UE_DEFINE_GAMEPLAY_TAG(TAG_StateMachine_Label_Default, "StateMachine.Label.Default");

/**
 * Simple RAII wrapper for UMachineState::bIsDispatchingEvent.
 * It queues certain users calls (GotoState, PushState, PopState) while dispatching event.
 */
struct FMS_IsDispatchingEventManager
{
public:
	explicit FMS_IsDispatchingEventManager(UMachineState* Context)
		: ContextRef(Context)
		, bIsDispatchingEvent(Context->bIsDispatchingEvent)
	{
		ensure(!bIsDispatchingEvent);
		bIsDispatchingEvent = true;
	}

	~FMS_IsDispatchingEventManager()
	{
		bIsDispatchingEvent = false;
		ensure(!bIsDispatchingEvent);

		if (ContextRef.IsValid() && ContextRef->OnFinishedDispatchingEvent.IsBound())
		{
			ContextRef->OnFinishedDispatchingEvent.Execute();
		}
	}

private:
	TWeakObjectPtr<UMachineState> ContextRef = nullptr;
	bool& bIsDispatchingEvent;
};

UMachineState::UMachineState()
{
	// Default place to define your custom machine state data class
	StateDataClass = UMachineStateData::StaticClass();

	// Default place to register all your labels
	REGISTER_LABEL(Default);
}

UMachineState::~UMachineState()
{
	StopRunningLabels();
	StopLatentExecution_Implementation();
}

bool UMachineState::IsStateValid() const
{
	return IsValid(this) && !bIsDestroyed;
}

bool UMachineState::IsDispatchingEvent() const
{
	return bIsDispatchingEvent;
}

FGameplayTag UMachineState::GetActiveLabel() const
{
	return ActiveLabel;
}

EStateAction UMachineState::GetLastStateAction() const
{
	return LastStateAction;
}

float UMachineState::GetTimeSinceLastStateAction() const
{
	const float TimeSinceLastStateAction = GetWorld()->GetTimeSeconds() - LastStateActionTime;
	return TimeSinceLastStateAction;
}

FString UMachineState::GetDebugData() const
{
	return "";
}

void UMachineState::OnBegan(TSubclassOf<UMachineState> OldState)
{
	OnAddedToStack(EStateAction::Begin, OldState);
	OnActivated(EStateAction::Begin, OldState);
}

void UMachineState::OnEnded(TSubclassOf<UMachineState> NewState)
{
	ensureMsgf(!bIsActivatingLabel, TEXT("Ending a state while a label is being activated is prohibited."));

	if (IsStateActive())
	{
		OnDeactivated(EStateAction::End, NewState);
	}

	OnRemovedFromStack(EStateAction::End, NewState);
}

void UMachineState::OnPushed(TSubclassOf<UMachineState> OldState)
{
	OnAddedToStack(EStateAction::Push, OldState);
	OnActivated(EStateAction::Push, OldState);
}

void UMachineState::OnPopped(TSubclassOf<UMachineState> NewState)
{
	OnDeactivated(EStateAction::Pop, NewState);
	OnRemovedFromStack(EStateAction::Pop, NewState);
}

void UMachineState::OnResumed(TSubclassOf<UMachineState> OldState)
{
	OnActivated(EStateAction::Resume, OldState);
}

void UMachineState::OnPaused(TSubclassOf<UMachineState> NewState)
{
	OnDeactivated(EStateAction::Pause, NewState);
}

void UMachineState::OnActivated(EStateAction StateAction, TSubclassOf<UMachineState> OldState)
{
	// Empty
}

void UMachineState::OnDeactivated(EStateAction StateAction, TSubclassOf<UMachineState> NewState)
{
	// Empty
}

void UMachineState::OnAddedToStack(EStateAction StateAction, TSubclassOf<UMachineState> OldState)
{
	// Empty
}

void UMachineState::OnRemovedFromStack(EStateAction StateAction, TSubclassOf<UMachineState> NewState)
{
	StopRunningLabels();
	StopLatentExecution_Implementation();
	ActiveLabel = TAG_StateMachine_Label_Default;
}

bool UMachineState::RegisterLabel(FGameplayTag Label, const FLabelSignature& Callback)
{
	if (!IsLabelTagCorrect(Label))
	{
		FSM_LOG(Warning, "Label [%s] is of wrong tag hierarchy.", *Label.ToString());
		return false;
	}

	if (!Callback.IsBound())
	{
		FSM_LOG(Warning, "Label [%s]'s callback is not bound.", *Label.ToString());
		return false;
	}

	if (ContainsLabel(Label))
	{
		FSM_LOG(Warning, "Label [%s] is not present in state [%s].", *Label.ToString(), *GetName());
		return false;
	}

	FSM_LOG(Log, "Label [%s] has been registered.", *Label.ToString());
	RegisteredLabels.Add(Label, Callback);
	return true;
}

int32 UMachineState::StopRunningLabels()
{
	int32 StoppedCoroutines = 0;
	for (auto& [Coroutine, DebugData] : RunningLabels)
	{
		if (Coroutine.IsDone())
		{
			continue;
		}

		Coroutine.Cancel();
		StoppedCoroutines++;

		FSM_LOG(VeryVerbose, "Label [%s] in state [%s] has been stopped.", *DebugData, *GetName());
	}

	if (StoppedCoroutines > 0)
	{
		FSM_LOG(Verbose, "All [%d] running coroutines in state [%s] have been cancelled.",
			StoppedCoroutines, *GetName());
	}

	RunningLabels.Empty();

	return StoppedCoroutines;
}

int32 UMachineState::ClearInvalidLatentExecutionCancellers()
{
	const int32 RemovedEntries = RunningLatentExecutions.RemoveAll([this](const FLatentExecution& Item)
	{
		if (!Item.CancelDelegate.IsBound())
		{
			FSM_LOG(VeryVerbose, "Secondary coroutine [%s] in state [%s] has been cleared up as it has finished the "
				"execution.", *Item.DebugData, *GetName());
			return true;
		}

		return false;
	});

	if (RemovedEntries > 0)
	{
		FSM_LOG(Verbose, "All [%d] running invalid latent execution cancellers in state [%s] have been cancelled.",
			RemovedEntries, *GetName());
	}

	return RemovedEntries;
}

int32 UMachineState::StopLatentExecution()
{
	if (!StateMachine.IsValid())
	{
		return -1;
	}

	const int32 Num = StateMachine->StopEveryLatentExecution();
	return Num;
}

void UMachineState::StopLatentExecution_Custom(int32 StoppedCoroutines)
{
	// Empty
}

int32 UMachineState::StopLatentExecution_Implementation()
{
	int32 StoppedCoroutines = 0;
	for (FLatentExecution& LatentExecution : RunningLatentExecutions)
	{
		if (LatentExecution.CancelDelegate.ExecuteIfBound())
		{
			FSM_LOG(VeryVerbose, "Secondary coroutine [%s] in state [%s] has been cancelled.",
				*LatentExecution.DebugData, *GetName());

			StoppedCoroutines++;
		}
	}

	if (StoppedCoroutines > 0)
	{
		FSM_LOG(Verbose, "All [%d] running secondary coroutines in state [%s] have been cancelled.",
			StoppedCoroutines, *GetName());
	}

	RunningLatentExecutions.Empty();

	// Allow users to do some custom clean up
	StopLatentExecution_Custom(StoppedCoroutines);

	return StoppedCoroutines;
}

bool UMachineState::IsStateActive() const
{
	return StateMachine->IsInState(GetClass());
}

void UMachineState::Tick(float DeltaSeconds)
{
	if (!bLabelActivated)
	{
		const FLabelSignature& LabelFunction = RegisteredLabels.FindChecked(ActiveLabel);
		if (ensureMsgf(LabelFunction.IsBound(), TEXT("Function for label [%s] is not bound"), *ActiveLabel.ToString()))
		{
			bLabelActivated = true;

			// Disallow editing the active label
			bIsActivatingLabel = true;

			const auto Coroutine = LabelFunction.Execute();
			RunningLabels.Add({ Coroutine, ActiveLabel.GetTagName().ToString() });

			FSM_LOG(Verbose, "State [%s] Label [%s] has been activated.",
				*GetClass()->GetName(), *ActiveLabel.ToString());

			// Re-allow editing the active label
			bIsActivatingLabel = false;
		}
	}
}

void UMachineState::Initialize()
{
	CreateStateData();
}

UMachineStateData* UMachineState::CreateStateData()
{
	check(!IsValid(BaseStateData));
	check(IsValid(StateDataClass));

	BaseStateData = NewObject<UMachineStateData>(this, StateDataClass, NAME_None, RF_Transient);

	FSM_LOG(Verbose, "Machine state data [%s] for state [%s] has been created.", *BaseStateData->GetName(), *GetName());

	return BaseStateData;
}

void UMachineState::SetStateMachine(UFiniteStateMachine* InStateMachine)
{
	check(StateMachine.IsExplicitlyNull());
	check(IsValid(InStateMachine));

	StateMachine = InStateMachine;

	Initialize();
}

void UMachineState::SetInitialLabel(FGameplayTag Label)
{
	ActiveLabel = Label;
	bLabelActivated = false;
}

void UMachineState::OnStateAction(EStateAction StateAction, TSubclassOf<UMachineState> StateClass)
{
	{
		FMS_IsDispatchingEventManager Guard(this);

		FSM_LOG(Log, "[%s] has been [%s].", *GetName(), *UEnum::GetValueAsString(StateAction));

		switch (StateAction)
		{
		case EStateAction::Begin:	OnBegan(StateClass); break;
		case EStateAction::End:		OnEnded(StateClass); break;
		case EStateAction::Push:	OnPushed(StateClass); break;
		case EStateAction::Pop:		OnPopped(StateClass); break;
		case EStateAction::Resume:	OnResumed(StateClass); break;
		case EStateAction::Pause:	OnPaused(StateClass); break;
		default: checkNoEntry();
		}

		// Save for debug purposes
		LastStateAction = StateAction;
		LastStateActionTime = GetTime();
	}

	// Notify about a state action
	OnStateActionDelegate.Broadcast(this, StateAction);
}

bool UMachineState::CanSafelyDeactivate(FString& OutReason) const
{
	if (bIsActivatingLabel)
	{
		OutReason = TEXT("Label is being activated. Try after it finishes (when UMachineState::Tick() ends).");
		return false;
	}

	return true;
}

TCoroutine<> UMachineState::Label_Default()
{
	co_return;
}

FString UMachineState::GetDebugString(const FString& RunLatentExecutionExt) const
{
	return FString::Printf(TEXT("State [%s] Owner [%s] RunLatentExecutionExt [%s]"),
		*GetNameSafe(this), *StateMachine.Get()->GetOwner()->GetName(), *RunLatentExecutionExt);
}

bool UMachineState::GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label, bool bForceEvents)
{
	return StateMachine->GotoState(InStateClass, Label, bForceEvents);
}

bool UMachineState::EndState()
{
	return StateMachine->EndState();
}

bool UMachineState::GotoLabel(FGameplayTag Label)
{
	if (Label.IsValid())
	{
		if (!IsLabelTagCorrect(Label))
		{
			FSM_LOG(Warning, "Label [%s] is of wrong tag hierarchy.", *Label.ToString());
			return false;
		}

		if (!ContainsLabel(Label))
		{
			FSM_LOG(Warning, "Label [%s] is not present in state [%s].", *Label.ToString(), *GetName());
			return false;
		}
	}

	// Don't try to activate an empty label. If it's empty, it means that no label should be running
	bLabelActivated = !Label.IsValid();
	ActiveLabel = Label;

	// Stop any latent code within a potential old label
	StopLatentExecution_Implementation();
	StopRunningLabels();

	return true;
}

TCoroutine<> UMachineState::PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label,
	bool* bOutPrematureResult)
{
	co_await StateMachine->PushState(InStateClass, Label, bOutPrematureResult);
}

TCoroutine<> UMachineState::PushStateQueued(FFSM_PushRequestHandle& OutHandle,
	TSubclassOf<UMachineState> InStateClass, FGameplayTag Label)
{
	co_await StateMachine->PushStateQueued(OutHandle, InStateClass, Label);
}

bool UMachineState::PopState()
{
	return StateMachine->PopState();
}

int32 UMachineState::ClearStack()
{
	return StateMachine->ClearStack();
}

bool UMachineState::ContainsLabel(FGameplayTag Label) const
{
	const FLabelSignature* LabelFunction = RegisteredLabels.Find(Label);
	return !!LabelFunction;
}

bool UMachineState::IsLabelTagCorrect(FGameplayTag Tag)
{
	return Tag.MatchesTag(TAG_StateMachine_Label);
}

AActor* UMachineState::GetOwner() const
{
	return GetOwner<AActor>();
}

float UMachineState::GetTime() const
{
	const UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
	const float Time = World ? World->GetTimeSeconds() : 0.0;
	return Time;
}

float UMachineState::TimeSince(float Time) const
{
	const float TimeSince = GetTime() - Time;
	return TimeSince;
}

FTimerManager& UMachineState::GetTimerManager() const
{
	const UWorld* World = GetWorld();
	check(IsValid(World));

	return World->GetTimerManager();
}
