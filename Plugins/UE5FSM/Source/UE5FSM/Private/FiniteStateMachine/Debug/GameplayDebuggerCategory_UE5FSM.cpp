#include "FiniteStateMachine/Debug/GameplayDebuggerCategory_UE5FSM.h"

#if WITH_UE5FSM_DEBUGGER

#include "Engine/Canvas.h"
#include "FiniteStateMachine/FiniteStateMachine.h"

FGameplayDebuggerCategory_UE5FSM::FGameplayDebuggerCategory_UE5FSM()
{
	bShowOnlyWithDebugActor = false;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_UE5FSM::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_UE5FSM());
}

static FSerializedFSMData SerializeFSMData(const UFiniteStateMachine* FiniteStateMachine)
{
	FSerializedFSMData ReturnValue;
	if (!IsValid(FiniteStateMachine))
	{
		return ReturnValue;
	}

	ReturnValue.GlobalStateClass = FiniteStateMachine->GetGlobalStateClass();
	if (const UMachineState* GlobalState = FiniteStateMachine->GetState(ReturnValue.GlobalStateClass))
	{
		ReturnValue.ExtGlobalDebugData = GlobalState->GetDebugData();
	}

	ReturnValue.RegisteredStateClasses = FiniteStateMachine->GetRegisteredStateClasses();
	const TArray<UFiniteStateMachine::FDebugStateAction> LastActions = FiniteStateMachine->GetLastStateActionsStack();
	for (const UFiniteStateMachine::FDebugStateAction Action : LastActions)
	{
		if (Action.Action == EStateAction::End || Action.Action == EStateAction::Pop)
		{
			ReturnValue.LastTerminatedStates.Add(Action);
		}
	}

	// Do not show that many entries, only show the last ones
	constexpr int32 MaxLastTerminatedEntries = 3;
	if (ReturnValue.LastTerminatedStates.Num() > MaxLastTerminatedEntries)
	{
		ReturnValue.LastTerminatedStates.SetNum(MaxLastTerminatedEntries);
	}

	const TArray<TSubclassOf<UMachineState>>& StateStack = FiniteStateMachine->GetStatesStack();
	auto It = StateStack.CreateConstIterator();
	It.SetToEnd();
	--It;

	for (; It; --It)
	{
		const UMachineState* State = FiniteStateMachine->GetState(*It);
		check(IsValid(State));

		FSerializedStateData& StateData = ReturnValue.StatesStack.AddDefaulted_GetRef();
		StateData.Name = State->GetName();
		StateData.LastAction = State->GetLastStateAction();
		StateData.TimeSinceLastStateAction = State->GetTimeSinceLastStateAction();
		StateData.ExtDebugData = State->GetDebugData();
	}

	return ReturnValue;
}

void FGameplayDebuggerCategory_UE5FSM::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (IsValid(DebugActor))
	{
		DebugData.Empty();

		const auto* ActorFSM = DebugActor->FindComponentByClass<UFiniteStateMachine>();
		if (IsValid(ActorFSM))
		{
			DebugData.Add(SerializeFSMData(ActorFSM));
		}

		if (const auto* Pawn = Cast<APawn>(DebugActor))
		{
			if (const auto* Controller = Pawn->GetController())
			{
				const auto* ControllerFSM = Controller->FindComponentByClass<UFiniteStateMachine>();
				if (IsValid(ControllerFSM))
				{
					DebugData.Add(SerializeFSMData(ControllerFSM));
				}
			}
		}
	}
}

constexpr FColor White(255, 255, 255);
constexpr FColor Gray(128, 128, 128);
constexpr FColor Yellow(255, 255, 0);
static FColor StateActionToColor(EStateAction StateAction)
{
	switch (StateAction)
	{
	case EStateAction::None: return Gray;
	case EStateAction::Begin: return White;
	case EStateAction::End: return Gray;
	case EStateAction::Push: return White;
	case EStateAction::Pop: return Gray;
	case EStateAction::Resume: return White;
	case EStateAction::Pause: return Yellow;
	default: return Gray;
	}
}

static void PrintGlobalState(const FSerializedFSMData& Data, FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.Printf(TEXT("Global state: %s"), *GetNameSafe(Data.GlobalStateClass));
}

static void PrintStatesStack(const FSerializedFSMData& Data, FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.Print(TEXT("\nStates stack:"));
	for (const FSerializedStateData& StateData : Data.StatesStack)
	{
		FString LastActionString = UEnum::GetValueAsString(StateData.LastAction);
		LastActionString.RemoveFromStart("EStateAction::");

		CanvasContext.Printf(StateActionToColor(StateData.LastAction), TEXT("- %s - %s (%.2fs)"),
			*StateData.Name, *LastActionString, StateData.TimeSinceLastStateAction);
	}
}

static void PrintRegisteredStates(const FSerializedFSMData& Data, FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.Print(TEXT("\nRegistered states:"));
	for (const TSubclassOf<UMachineState> StateClass : Data.RegisteredStateClasses)
	{
		CanvasContext.Printf(TEXT("- %s"), *StateClass->GetName());
	}
}

static void PrintTerminatedStates(const FSerializedFSMData& Data, FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (Data.LastTerminatedStates.IsEmpty())
	{
		return;
	}

	CanvasContext.Print(TEXT("\nLast terminated states:"));
	for (const UFiniteStateMachine::FDebugStateAction& StateData : Data.LastTerminatedStates)
	{
		FString LastActionString = UEnum::GetValueAsString(StateData.Action);
		LastActionString.RemoveFromStart("EStateAction::");

		const float TimeSinceAction = CanvasContext.GetWorld()->GetTimeSeconds() - StateData.ActionTime;
		CanvasContext.Printf(StateActionToColor(StateData.Action), TEXT("- %s - %s (%.2fs)"),
			*GetNameSafe(StateData.State.Get()), *LastActionString, TimeSinceAction);
	}
}

static void PrintExtDebugData(const FSerializedFSMData& Data, FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (Data.ExtGlobalDebugData.IsEmpty() && Data.StatesStack.IsEmpty())
	{
		return;
	}

	CanvasContext.Print(TEXT("\nExtended debug data:"));
	if (!Data.ExtGlobalDebugData.IsEmpty())
	{
		CanvasContext.Printf(TEXT("\nGlobal data:\n%s"), *Data.ExtGlobalDebugData);
	}

	for (const FSerializedStateData& StateClass : Data.StatesStack)
	{
		if (!StateClass.ExtDebugData.IsEmpty())
		{
			CanvasContext.Printf(TEXT("- %s - %s"), *StateClass.Name, *StateClass.ExtDebugData);
		}
	}

}

void FGameplayDebuggerCategory_UE5FSM::DrawData(APlayerController* OwnerPC,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	for (const FSerializedFSMData& Data : DebugData)
	{
		PrintGlobalState(Data, CanvasContext);
		PrintStatesStack(Data, CanvasContext);
		PrintTerminatedStates(Data, CanvasContext);
		PrintRegisteredStates(Data, CanvasContext);
		PrintExtDebugData(Data, CanvasContext);
	}
}

#endif