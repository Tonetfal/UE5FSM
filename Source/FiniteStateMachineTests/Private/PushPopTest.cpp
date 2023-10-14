#include "Editor.h"
#include "FiniteStateMachine/FiniteStateMachine.h"
#include "FiniteStateMachineTestObject.h"
#include "MachineState_PushPopTest.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#define LATENT_TEST_TRUE(What, Value)\
	if (!Test->TestTrue(What, Value))\
	{\
		return true;\
	}

#define LATENT_TEST_FALSE(What, Value)\
	if (!Test->TestFalse(What, Value))\
	{\
		return true;\
	}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineBasicTest, "FiniteStateMachine.BasicPushPop",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineErrorTest, "FiniteStateMachine.ErrorPushPop",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineLatentTest, "FiniteStateMachine.LatentPushPop",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

#pragma region LatentAutomationCommands

#define PREDICTED_STATE_ACTION(MACHINE_STATE_CLASS, ACTION) { MACHINE_STATE_CLASS::StaticClass(), EStateAction::ACTION }
#define LATENT_TEST_BEGIN() \
	LATENT_TEST_TRUE("Test actor is valid", TestActor != nullptr && IsValid(*TestActor) && IsValid((*TestActor)->StateMachine)); \
	UFiniteStateMachine* StateMachine = (*TestActor)->StateMachine

AFiniteStateMachineTestActor* TestActor = nullptr;
static void StartTest()
{
	// Clear old results
	Actions.Empty();

	// Listen for actions
	if (!ActionDelegate.IsBound())
	{
		ActionDelegate.BindLambda([] (TSubclassOf<UMachineState> MachineState, EStateAction StateAction)
		{
			Actions.Add({ MachineState, StateAction });
		});
	}
}

DEFINE_LATENT_AUTOMATION_COMMAND(FStartLatentTest);
bool FStartLatentTest::Update()
{
	StartTest();
	return true;
}

static void EndTest()
{
}

DEFINE_LATENT_AUTOMATION_COMMAND(FEndLatentTest);
bool FEndLatentTest::Update()
{
	EndTest();
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FCreateTestActor,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor);
bool FCreateTestActor::Update()
{
	LATENT_TEST_TRUE("Test actor setup correct", TestActor != nullptr);

	const FWorldContext* WorldContext = GEditor->GetPIEWorldContext();
	if (WorldContext)
	{
		UWorld* World = WorldContext->World();
		if (World)
		{
			*TestActor = World->SpawnActor<AFiniteStateMachineTestActor>();
		}
	}

	LATENT_TEST_TRUE("Test actor created", IsValid(*TestActor));
	LATENT_TEST_TRUE("State machine created", IsValid((*TestActor)->StateMachine));
	LATENT_TEST_TRUE("State machine initialized", (*TestActor)->StateMachine->HasBeenInitialized());
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FRegisterState,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass);
bool FRegisterState::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Register state", StateMachine->RegisterState(StateClass));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FRegisterState_Fail,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass);
bool FRegisterState_Fail::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Fail register state", !StateMachine->RegisterState(StateClass));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FIsStateRegistered,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass);
bool FIsStateRegistered::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("State registered", StateMachine->IsStateRegistered(StateClass));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FIsNotStateRegistered,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass);
bool FIsNotStateRegistered::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("State not registered", !StateMachine->IsStateRegistered(StateClass));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FGotoState,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass,
	FGameplayTag, Label);
bool FGotoState::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Go to state", StateMachine->GotoState(StateClass, Label));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FGotoStateNoForce,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass,
	FGameplayTag, Label);
bool FGotoStateNoForce::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Go to state", StateMachine->GotoState(StateClass, Label, false));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FGotoState_Fail,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass,
	FGameplayTag, Label);
bool FGotoState_Fail::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Fail go to state", !StateMachine->GotoState(StateClass, Label));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FIsInState,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass);
bool FIsInState::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Is in state", StateMachine->IsInState(StateClass));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FIsNotInState,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass);
bool FIsNotInState::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Is not in state", !StateMachine->IsInState(StateClass));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FPushState,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass,
	FGameplayTag, Label);
bool FPushState::Update()
{
	LATENT_TEST_BEGIN();

	bool PushResult;
	StateMachine->PushState(StateClass, Label, &PushResult);
	LATENT_TEST_TRUE("Push state", PushResult);
	LATENT_TEST_TRUE("Is pushed state active?", StateMachine->IsInState(StateClass));

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FPushState_Fail,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass,
	FGameplayTag, Label);
bool FPushState_Fail::Update()
{
	LATENT_TEST_BEGIN();

	bool PushResult;
	StateMachine->PushState(StateClass, Label, &PushResult);
	LATENT_TEST_TRUE("Fail to push state", !PushResult);

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FPopState,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor,
	TSubclassOf<UMachineState>, ResumedStateClass);
bool FPopState::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Pop state", StateMachine->PopState());
	if (ResumedStateClass)
	{
		LATENT_TEST_TRUE("Is state active?", StateMachine->IsInState(ResumedStateClass));
	}

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FPopState_Fail,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor,
	TSubclassOf<UMachineState>, ResumedStateClass);
bool FPopState_Fail::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Fail to pop state", !StateMachine->PopState());
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FCompareActions,
	FAutomationTestBase*, Test, const TArray<FMachineActionPair>&, PredictedStateActions);
bool FCompareActions::Update()
{
	Test->TestTrue("Same amount of machine actions", Actions.Num() == PredictedStateActions.Num());

	auto LhsStateActionsIt = Actions.CreateConstIterator();
	auto RhsStateActionsIt = PredictedStateActions.CreateConstIterator();
	for (int32 i = 0; LhsStateActionsIt && RhsStateActionsIt; ++LhsStateActionsIt, ++RhsStateActionsIt, i++)
	{
		Test->TestTrue(FString("Single action machine states are the same. Iteration: ") + FString::FormatAsNumber(i),
			LhsStateActionsIt->Class == RhsStateActionsIt->Class);
		Test->TestTrue(FString("Single action machine actions are the same. Iteration: ") + FString::FormatAsNumber(i),
			LhsStateActionsIt->Action == RhsStateActionsIt->Action);
	}

	LATENT_TEST_TRUE("State actions are the same", !LhsStateActionsIt && !RhsStateActionsIt);
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FStartPushPopLatentTest);
TArray<FPushPopLatentMessage> LatentMessages;
bool FStartPushPopLatentTest::Update()
{
	FStartLatentTest NativeStartTestCommand;
	if (!NativeStartTestCommand.Update())
	{
		return true;
	}

	LatentMessages.Empty();
	OnMessageDelegate.Clear();
	OnMessageDelegate.AddLambda([] (FPushPopLatentMessage Message)
	{
		LatentMessages.Add(Message);
	});

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FWaitPushPopMessage, FAutomationTestBase*, Test, FString, Message,
	float, MaxDuration, bool, bFirstIteration);
bool FWaitPushPopMessage::Update()
{
	// This command supports only one execution at a time
	// This is a little hack as it would take a bit more time to make it work properly :(
	constexpr float InvalidTime = -1.f;
	static float BeginTime = InvalidTime;
	static bool bGotMessage = false;

	if (bGotMessage)
	{
		bGotMessage = false;
		BeginTime = InvalidTime;
		return true;
	}

	const FWorldContext* WorldContext = GEditor->GetPIEWorldContext();
	if (!WorldContext)
	{
		BeginTime = InvalidTime;
		return true;
	}

	const UWorld* World = WorldContext->World();
	if (!World)
	{
		BeginTime = InvalidTime;
		return true;
	}

	const float CurrentTime = World->GetRealTimeSeconds();
	if (bFirstIteration)
	{
		ensure(BeginTime == InvalidTime);
		bFirstIteration = false;
		BeginTime = CurrentTime;

		OnMessageDelegate.AddLambda([&] (FPushPopLatentMessage InMessage)
		{
			if (Message == InMessage.Message)
			{
				bGotMessage = true;
			}
		});
	}

	const float DeltaTime = CurrentTime - BeginTime;
	if (!Test->TestTrue("Waiting push pop message time didn't expire", MaxDuration >= DeltaTime))
	{
		BeginTime = InvalidTime;
		return true;
	}

	return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FComparePushPopMessages,
	FAutomationTestBase*, Test, const TArray<FPushPopLatentMessage>&, DemandedMessages);

bool FComparePushPopMessages::Update()
{
	// All messages must notify only success
	for (const auto& [Class, Message, bSuccess] : LatentMessages)
	{
		Test->TestTrue(Message, bSuccess);
	}

	// Check whether all demanded messages were sent to sue
	for (const auto& [DemandedClass, DemandedMessage, _] : DemandedMessages)
	{
		bool bMessageFound = false;
		for (const auto& [OtherClass, OtherMessage, __] : LatentMessages)
		{
			if (DemandedClass == OtherClass && DemandedMessage == OtherMessage)
			{
				bMessageFound = true;
				break;
			}
		}

		const FString Message = FString::Printf(TEXT("\"%s\" from %s"), *DemandedMessage, *DemandedClass->GetName());
		Test->TestTrue(Message, bMessageFound);
	}

	return true;
}

#pragma endregion

bool FFiniteStateMachineBasicTest::RunTest(const FString& Parameters)
{
	static TArray<FMachineActionPair> PredictedStateActions;
	PredictedStateActions.Empty();

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	// Register states
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_Test1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FIsStateRegistered(this, &TestActor, UMachineState_Test1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_Test2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FIsStateRegistered(this, &TestActor, UMachineState_Test2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_Test3::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FIsStateRegistered(this, &TestActor, UMachineState_Test3::StaticClass()));

	// Interact with the states stack
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, UMachineState_Test2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, UMachineState_Test1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));

	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Begin));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Pause));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test2, Push));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test2, Pause));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test3, Push));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test3, Pop));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test2, Resume));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test2, Pop));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Resume));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Pop));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareActions(this, PredictedStateActions));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

bool FFiniteStateMachineErrorTest::RunTest(const FString& Parameters)
{
	static TArray<FMachineActionPair> PredictedStateActions;
	PredictedStateActions.Empty();

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	// Register states
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_Test1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FIsStateRegistered(this, &TestActor, UMachineState_Test1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_Test2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FIsStateRegistered(this, &TestActor, UMachineState_Test2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_Test3::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FIsStateRegistered(this, &TestActor, UMachineState_Test3::StaticClass()));

	// Try to fail to register states that are already registered
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState_Fail(this, &TestActor, UMachineState_Test1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState_Fail(this, &TestActor, UMachineState_Test2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState_Fail(this, &TestActor, UMachineState_Test3::StaticClass()));

	/** Interact with the states stack */

	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_Test1::StaticClass()));

	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Begin));

	// Going to the active state is allowed
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_Test1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));
	ADD_LATENT_AUTOMATION_COMMAND(FIsNotInState(this, &TestActor, UMachineState_Test1::StaticClass()));

	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, End));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Begin));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Pop));

	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));

	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test2, Push));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test2, Pause));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test3, Push));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test3, Pause));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Push));

	// Disallow pushing a state that is already present
	ADD_LATENT_AUTOMATION_COMMAND(FPushState_Fail(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState_Fail(this, &TestActor, UMachineState_Test2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState_Fail(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));

	// But allow to goto to the top-most state
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, End));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Begin));

	// Disallow going to a state that is already present below the top-most state in the stack
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState_Fail(this, &TestActor, UMachineState_Test2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState_Fail(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));

	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, UMachineState_Test3::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, UMachineState_Test2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));

	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test1, Pop));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test3, Resume));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test3, Pop));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test2, Resume));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test2, Pop));

	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_Test3::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));

	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test3, Begin));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_Test3, Pop));

	// Popping with empty stack should result in no action
	ADD_LATENT_AUTOMATION_COMMAND(FPopState_Fail(this, &TestActor, nullptr));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState_Fail(this, &TestActor, nullptr));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareActions(this, PredictedStateActions));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

bool FFiniteStateMachineLatentTest::RunTest(const FString& Parameters)
{
	static TArray<FMachineActionPair> PredictedStateActions;
	PredictedStateActions.Empty();

	static const TArray<FPushPopLatentMessage> DemandedMessages
	{
		{ UMachineState_PushPopTest1::StaticClass(), "Timer delay works", true },
		{ UMachineState_PushPopTest2::StaticClass(), "Timer delay works", true },
		{ UMachineState_PushPopTest3::StaticClass(), "Timer delay works", true }
	};

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartPushPopLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	// Register states
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_PushPopTest1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FIsStateRegistered(this, &TestActor, UMachineState_PushPopTest1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_PushPopTest2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FIsStateRegistered(this, &TestActor, UMachineState_PushPopTest2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_PushPopTest3::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FIsStateRegistered(this, &TestActor, UMachineState_PushPopTest3::StaticClass()));

	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest1, Begin));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest1, Pause));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest2, Push));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest2, Pause));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest3, Push));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest3, Pop));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest2, Resume));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest2, Pop));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest1, Resume));
	PredictedStateActions.Add(PREDICTED_STATE_ACTION(UMachineState_PushPopTest1, Pop));

	// The PushPopTest states push/pop in the correct order themselves, we're only left with listening for messages they send
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_PushPopTest1::StaticClass(), TAG_StateMachine_Label_Default));

	// Wait until we're notified that the test has ended
	ADD_LATENT_AUTOMATION_COMMAND(FWaitPushPopMessage(this, "Test finished", 20.f, true));

	// Wait some time until last state pops itself
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareActions(this, PredictedStateActions));
	ADD_LATENT_AUTOMATION_COMMAND(FComparePushPopMessages(this, DemandedMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}
