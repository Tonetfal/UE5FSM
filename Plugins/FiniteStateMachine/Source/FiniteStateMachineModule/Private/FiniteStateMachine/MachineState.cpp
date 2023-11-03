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

void UMachineState::Begin(TSubclassOf<UMachineState> PreviousState)
{
	// Empty
}

void UMachineState::End(TSubclassOf<UMachineState> NewState)
{
	UE_LOG(LogFiniteStateMachine, Verbose, TEXT("[%s] end."), *GetName());

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

	FSM_LOG(Verbose, "Label [%s] has been registered.", *Label.ToString());
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

		FSM_LOG(Verbose, "Label [%s] in state [%s] has been stopped.", *DebugData, *GetName());
	}

	if (StoppedCoroutines > 0)
	{
		FSM_LOG(VeryVerbose, "All [%d] running coroutines in state [%s] have been cancelled.",
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
			FSM_LOG(Verbose, "Secondary coroutine [%s] in state [%s] has been cleared up as it has finished the "
				"execution.", *Item.DebugData, *GetName());
			return true;
		}

		return false;
	});

	if (RemovedEntries > 0)
	{
		FSM_LOG(VeryVerbose, "All [%d] running invalid latent execution cancellers in state [%s] have been cancelled.",
			RemovedEntries, *GetName());
	}

	return RemovedEntries;
}

int32 UMachineState::StopLatentExecution()
{
	return StateMachine->StopEveryLatentExecution();
	if (!StateMachine.IsValid())
	{
		return -1;
	}

	const int32 Num = StateMachine->StopEveryLatentExecution();
	StopLatentExecution_Custom();
	return Num;
}
}

int32 UMachineState::StopLatentExecution_Implementation()
{
	int32 StoppedCoroutines = 0;
	for (FSimpleDelegate& RunningLatentExecution : RunningLatentExecutions)
	{
		const bool bExecuted = RunningLatentExecution.ExecuteIfBound();
		if (bExecuted)
		{
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
		if (ensure(LabelFunction.IsBound()))
		{
			bLabelActivated = true;
			RunningLabelCoroutines.Add(LabelFunction.Execute());
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
	FSM_LOG(Verbose, "[%s] has been [%s].", *GetName(), *UEnum::GetValueAsString(StateAction));

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

	// Notify about a state action
	OnStateActionDelegate.Broadcast(StateAction);
}

TCoroutine<> UMachineState::Label_Default()
{
	co_return;
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
