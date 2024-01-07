#include "FiniteStateMachine/MachineState.h"

#include "FiniteStateMachine/FiniteStateMachine.h"
#include "FiniteStateMachine/FiniteStateMachineTypes.h"
#include "FiniteStateMachine/MachineStateData.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateMachine_Label, "StateMachine.Label");
UE_DEFINE_GAMEPLAY_TAG(TAG_StateMachine_Label_Default, "StateMachine.Label.Default");

#ifdef FSM_EXTEREME_VERBOSITY
	#define FSM_EXTEREME_VERBOSITY_STR FString::Printf(TEXT("State [%s] Owner [%s]"), \
		*GetNameSafe(this), StateMachine.IsValid() ? *GetNameSafe(StateMachine->GetOwner()) : TEXT("Invalid"))
#else
	#define FSM_EXTEREME_VERBOSITY_STR TEXT("")
#endif

#define FSM_LOG(VERBOSITY, MESSAGE, ...) UE_LOG(LogFiniteStateMachine, VERBOSITY, TEXT("%s - %s"), \
	*FSM_EXTEREME_VERBOSITY_STR, *FString::Printf(TEXT(MESSAGE), ## __VA_ARGS__))

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

void UMachineState::Begin(TSubclassOf<UMachineState> PreviousState)
{
	// Empty
}

void UMachineState::End(TSubclassOf<UMachineState> NewState)
{
	ensureMsgf(!bIsActivatingLabel, TEXT("Ending a state while a label is being activated is prohibited."));

	StopRunningLabels();
	StopLatentExecution_Implementation();
	ActiveLabel = TAG_StateMachine_Label_Default;
}

void UMachineState::Pushed()
{
	// Empty
}

void UMachineState::Popped()
{
	StopRunningLabels();
	StopLatentExecution_Implementation();
	ActiveLabel = TAG_StateMachine_Label_Default;
}

void UMachineState::Paused()
{
	// Empty
}

void UMachineState::Resumed()
{
	// Empty
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
	StopLatentExecution_Custom();
	return Num;
}

void UMachineState::StopLatentExecution_Custom()
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

void UMachineState::OnStateAction(EStateAction StateAction, void* OptionalData)
{
	FSM_LOG(Log, "[%s] has been [%s].", *GetName(), *UEnum::GetValueAsString(StateAction));

	switch (StateAction)
	{
		case EStateAction::Begin:	Begin(static_cast<UClass*>(OptionalData)); break;
		case EStateAction::End:		End(static_cast<UClass*>(OptionalData)); break;
		case EStateAction::Push:	Pushed(); break;
		case EStateAction::Pop:		Popped(); break;
		case EStateAction::Resume:	Resumed(); break;
		case EStateAction::Pause:	Paused(); break;
		default: checkNoEntry();
	}

	// Save for debug purposes
	LastStateAction = StateAction;
	LastStateActionTime = GetWorld()->GetTimeSeconds();

	// Notify about a state action
	OnStateActionDelegate.Broadcast(StateAction);
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

bool UMachineState::GotoState(TSubclassOf<UMachineState> InStateClass,
	FGameplayTag Label /*TAG_StateMachine_Label_Begin*/)
{
	return StateMachine->GotoState(InStateClass, Label);
}

bool UMachineState::GotoLabel(FGameplayTag Label)
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

	ActiveLabel = Label;
	bLabelActivated = false;
	return true;
}

TCoroutine<> UMachineState::PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label)
{
	co_await StateMachine->PushState(InStateClass, Label);
}

bool UMachineState::PopState()
{
	return StateMachine->PopState();
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

FTimerManager& UMachineState::GetTimerManager()
{
	const UWorld* World = GetWorld();
	check(IsValid(World));

	return World->GetTimerManager();
}
