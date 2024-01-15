#include "Editor.h"
#include "FiniteStateMachine/FiniteStateMachine.h"
#include "FiniteStateMachineTestObject.h"
#include "MachineState_BlockedPushTest.h"
#include "MachineState_ExternalPushPopTest.h"
#include "MachineState_ExternalPushTest.h"
#include "MachineState_LatentActions.h"
#include "MachineState_LatentTest.h"
#include "MachineState_PushPopTest.h"
#include "MachineState_StartWithNotDefaultLabel.h"
#include "MachineState_StatesBlocklistTest.h"
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

#pragma region LatentAutomationCommands

#define PREDICTED_TEST_MESSAGE(MACHINE_STATE_CLASS, MESSAGE, SUCCESS) { MACHINE_STATE_CLASS::StaticClass(), MESSAGE, SUCCESS }
#define LATENT_TEST_BEGIN() \
	LATENT_TEST_TRUE("Test actor is valid", TestActor != nullptr && IsValid(*TestActor) && IsValid((*TestActor)->StateMachine)); \
	UFiniteStateMachine* StateMachine = (*TestActor)->StateMachine

AFiniteStateMachineTestActor* TestActor = nullptr;

TArray<FStateMachineTestMessage> LatentMessages;
void StartTest()
{
	LatentMessages.Empty();
	OnMessageDelegate.Clear();
	OnMessageDelegate.AddLambda([] (FStateMachineTestMessage Message)
	{
		LatentMessages.Add(Message);
	});
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

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FPushStateV2,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass,
	FGameplayTag, Label);
bool FPushStateV2::Update()
{
	LATENT_TEST_BEGIN();

	bool PushResult;
	StateMachine->PushState(StateClass, Label, &PushResult);
	LATENT_TEST_TRUE("Push state", PushResult);

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FIVE_PARAMETER(FPushStateToQueue,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, TSubclassOf<UMachineState>, StateClass,
	FGameplayTag, Label, bool, bWillBePending);
bool FPushStateToQueue::Update()
{
	LATENT_TEST_BEGIN();

	FFSM_PushRequestHandle Handle;
	StateMachine->PushStateQueued(Handle, StateClass, Label);
	LATENT_TEST_TRUE("Push state", Handle.IsPending() == bWillBePending);

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

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FClearStack,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor, int32, StatesEnded);
bool FClearStack::Update()
{
	LATENT_TEST_BEGIN();

	const int32 Num = StateMachine->ClearStack();
	LATENT_TEST_TRUE("All states have ended", Num == StatesEnded);

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FEndState,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor,
	TSubclassOf<UMachineState>, ResumedStateClass);
bool FEndState::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("End state", StateMachine->EndState());
	if (ResumedStateClass)
	{
		LATENT_TEST_TRUE("Is state active?", StateMachine->IsInState(ResumedStateClass));
	}

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FPopStateV2,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor);
bool FPopStateV2::Update()
{
	LATENT_TEST_BEGIN();
	LATENT_TEST_TRUE("Pop state", StateMachine->PopState());

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

		OnMessageDelegate.AddLambda([&] (FStateMachineTestMessage InMessage)
		{
			if (Message == InMessage.Message)
			{
				bGotMessage = true;
			}
		});
	}

	const float DeltaTime = CurrentTime - BeginTime;
	if (!Test->TestTrue("Waiting latent message time didn't expire", MaxDuration >= DeltaTime))
	{
		BeginTime = InvalidTime;
		return true;
	}

	return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FCompareTestMessages,
	FAutomationTestBase*, Test, const TArray<FStateMachineTestMessage>&, ExpectedTestMessages);

bool FCompareTestMessages::Update()
{
	Test->TestTrue("Expected and received test messages are of the same amount",
		LatentMessages.Num() == ExpectedTestMessages.Num());

	auto LhsStateActionsIt = LatentMessages.CreateConstIterator();
	auto RhsStateActionsIt = ExpectedTestMessages.CreateConstIterator();
	for (int32 i = 0; LhsStateActionsIt && RhsStateActionsIt; ++LhsStateActionsIt, ++RhsStateActionsIt, i++)
	{
		Test->TestTrue(FString("Single test messages are the same. Iteration: ") + FString::FormatAsNumber(i),
			LhsStateActionsIt->Class == RhsStateActionsIt->Class);
		Test->TestTrue(FString("Single test messages are the same. Iteration: ") + FString::FormatAsNumber(i),
			LhsStateActionsIt->Message == RhsStateActionsIt->Message);
		Test->TestTrue(FString("Single test messages are the same. Iteration: ") + FString::FormatAsNumber(i),
			LhsStateActionsIt->bSuccess == RhsStateActionsIt->bSuccess);
	}

	// Print all the broadcast and expected messages to facilitate testing
	// {
	// 	auto RhsPrintStateActionsIt = ExpectedTestMessages.CreateConstIterator();
	// 	UE_LOG(LogTemp, Warning, TEXT("What we expect: "));
	// 	for (int32 i = 0; RhsPrintStateActionsIt; ++RhsPrintStateActionsIt, i++)
	// 	{
	// 		UE_LOG(LogTemp, Warning, TEXT("%d. [%s] [%s]"),
	// 			i, *GetNameSafe(RhsPrintStateActionsIt->Class), *RhsPrintStateActionsIt->Message);
	// 	}
	//
	// 	auto LhsPrintStateActionsIt = LatentMessages.CreateConstIterator();
	// 	UE_LOG(LogTemp, Warning, TEXT("What we got: "));
	// 	for (int32 i = 0; LhsPrintStateActionsIt; ++LhsPrintStateActionsIt, i++)
	// 	{
	// 		UE_LOG(LogTemp, Warning, TEXT("%d. [%s] [%s]"),
	// 			i, *GetNameSafe(LhsPrintStateActionsIt->Class), *LhsPrintStateActionsIt->Message);
	// 	}
	// }

	LATENT_TEST_TRUE("Test messages are the same", !LhsStateActionsIt && !RhsStateActionsIt);

	return true;
}

#pragma endregion

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineBasicTest, "UE5FSM.BasicPushPop",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineBasicTest::RunTest(const FString& Parameters)
{
	static const TArray<FStateMachineTestMessage> ExpectedTestMessages
	{
		{ UMachineState_Test1::StaticClass(), "Begin", true },
		{ UMachineState_Test1::StaticClass(), "Paused", true },
		{ UMachineState_Test2::StaticClass(), "Pushed", true },
		{ UMachineState_Test2::StaticClass(), "Paused", true },
		{ UMachineState_Test3::StaticClass(), "Pushed", true },
		{ UMachineState_Test3::StaticClass(), "Popped", true },
		{ UMachineState_Test2::StaticClass(), "Resumed", true },
		{ UMachineState_Test2::StaticClass(), "Popped", true },
		{ UMachineState_Test1::StaticClass(), "Resumed", true },
		{ UMachineState_Test1::StaticClass(), "Popped", true },
	};

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

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineErrorTest, "UE5FSM.ErrorPushPop",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineErrorTest::RunTest(const FString& Parameters)
{
	static TArray<FStateMachineTestMessage> ExpectedTestMessages;
	ExpectedTestMessages.Empty();

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

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test1, "Begin", true));

	// Going to the active state is allowed
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_Test1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));
	ADD_LATENT_AUTOMATION_COMMAND(FIsNotInState(this, &TestActor, UMachineState_Test1::StaticClass()));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test1, "End", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test1, "Begin", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test1, "Popped", true));

	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test2, "Pushed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test2, "Paused", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test3, "Pushed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test3, "Paused", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test1, "Pushed", true));

	// Disallow pushing a state that is already present
	ADD_LATENT_AUTOMATION_COMMAND(FPushState_Fail(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState_Fail(this, &TestActor, UMachineState_Test2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState_Fail(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));

	// But allow to goto to the top-most state
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test1, "End", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test1, "Begin", true));

	// Disallow going to a state that is already present below the top-most state in the stack
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState_Fail(this, &TestActor, UMachineState_Test2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState_Fail(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));

	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, UMachineState_Test3::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, UMachineState_Test2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test1, "Popped", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test3, "Resumed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test3, "Popped", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test2, "Resumed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test2, "Popped", true));

	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_Test3::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test3, "Begin", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_Test3, "Popped", true));

	// Popping with empty stack should result in no action
	ADD_LATENT_AUTOMATION_COMMAND(FPopState_Fail(this, &TestActor, nullptr));
	ADD_LATENT_AUTOMATION_COMMAND(FPopState_Fail(this, &TestActor, nullptr));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineLatentTest, "UE5FSM.LatentPushPop",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineLatentTest::RunTest(const FString& Parameters)
{
	// THSI TEST DOES NOT SHOW HOW TO CORRECTLY USE THE LABELS & GOTO STATE, BUT MAKES SURE THAT IT WORKS AS IT'S
	// EXPECTED WHEN IT'S MISUSED


	static const TArray<FStateMachineTestMessage> ExpectedTestMessages
	{
		{ UMachineState_PushPopTest1::StaticClass(), "Begin", true },
		{ UMachineState_PushPopTest1::StaticClass(), "Before test label activation", true },
		{ UMachineState_PushPopTest1::StaticClass(), "Paused", true },
		{ UMachineState_PushPopTest2::StaticClass(), "Pushed", true },
		{ UMachineState_PushPopTest2::StaticClass(), "Before test label activation", true },
		{ UMachineState_PushPopTest2::StaticClass(), "Paused", true },
		{ UMachineState_PushPopTest3::StaticClass(), "Pushed", true },
		{ UMachineState_PushPopTest3::StaticClass(), "Before test label activation", true },
		{ UMachineState_PushPopTest3::StaticClass(), "Latent execution has been cancelled", true },
		{ UMachineState_PushPopTest3::StaticClass(), "Popped", true },
		{ UMachineState_PushPopTest2::StaticClass(), "Resumed", true },
		{ UMachineState_PushPopTest2::StaticClass(), "Latent execution has been cancelled", true },
		{ UMachineState_PushPopTest2::StaticClass(), "Popped", true },
		{ UMachineState_PushPopTest1::StaticClass(), "Resumed", true },
		{ UMachineState_PushPopTest1::StaticClass(), "Latent execution has been cancelled", true },
		{ UMachineState_PushPopTest1::StaticClass(), "Popped", true },
		{ UMachineState_PushPopTest1::StaticClass(), "End test", true }
	};

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
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

	// The PushPopTest states push/pop in the correct order themselves, we're only left with listening for messages they send
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_PushPopTest1::StaticClass(), TAG_StateMachine_Label_Default));

	// Wait until we're notified that the test has ended
	ADD_LATENT_AUTOMATION_COMMAND(FWaitPushPopMessage(this, "End test", 20.f, true));

	// Wait some time until last state pops itself
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineLatentGotoStateTest, "UE5FSM.LatentGotoState",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineLatentGotoStateTest::RunTest(const FString& Parameters)
{
	static const TArray<FStateMachineTestMessage> ExpectedTestMessages
	{
		{ UMachineState_GotoStateTest1::StaticClass(), "Begin", true },
		{ UMachineState_GotoStateTest1::StaticClass(), "Goto test 2 fail", true },
		{ UMachineState_GotoStateTest1::StaticClass(), "Pre goto test 2", true },
		{ UMachineState_GotoStateTest1::StaticClass(), "End", true },
		{ UMachineState_GotoStateTest2::StaticClass(), "Begin", true },
		{ UMachineState_GotoStateTest2::StaticClass(), "End test", true },
	};

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	// Register states
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_GotoStateTest1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_GotoStateTest2::StaticClass()));

	// The goto state test states do everything inside, we're only left with listening for messages they send
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_GotoStateTest1::StaticClass(), TAG_StateMachine_Label_Default));

	// Wait until we're notified that the test has ended
	ADD_LATENT_AUTOMATION_COMMAND(FWaitPushPopMessage(this, "End test", 2.f, true));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineLatentExecutionTest, "UE5FSM.LatentExecution",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineLatentExecutionTest::RunTest(const FString& Parameters)
{
	static const TArray<FStateMachineTestMessage> ExpectedTestMessages
	{
		{ UMachineState_LatentExecutionTest1::StaticClass(), "Begin", true },
		{ UMachineState_LatentExecutionTest1::StaticClass(), "Pre sleep", true },
		{ UMachineState_LatentExecutionTest1::StaticClass(), "Pre push latent execution test 2", true },
		{ UMachineState_LatentExecutionTest1::StaticClass(), "Paused", true },
		{ UMachineState_LatentExecutionTest2::StaticClass(), "Pushed", true },
		{ UMachineState_LatentExecutionTest1::StaticClass(), "Post push latent execution test 2", true },
		{ UMachineState_LatentExecutionTest2::StaticClass(), "Pre sleep", true },
		{ UMachineState_LatentExecutionTest2::StaticClass(), "Post sleep", true },
		{ UMachineState_LatentExecutionTest2::StaticClass(), "Pre pop", true },
		{ UMachineState_LatentExecutionTest2::StaticClass(), "Popped", true },
		{ UMachineState_LatentExecutionTest1::StaticClass(), "Resumed", true },
		{ UMachineState_LatentExecutionTest1::StaticClass(), "Post sleep", true },
		{ UMachineState_LatentExecutionTest1::StaticClass(), "End test", true },
	};

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	// Register states
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_LatentExecutionTest1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_LatentExecutionTest2::StaticClass()));

	// The goto state test states do everything inside, we're only left with listening for messages they send
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_LatentExecutionTest1::StaticClass(), TAG_StateMachine_Label_Default));

	// Wait until we're notified that the test has ended
	ADD_LATENT_AUTOMATION_COMMAND(FWaitPushPopMessage(this, "End test", 6.f, true));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineStatesBlocklistTest, "UE5FSM.StatesBlocklist",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineStatesBlocklistTest::RunTest(const FString& Parameters)
{
	static const TArray<FStateMachineTestMessage> ExpectedTestMessages
	{
		{ UMachineState_StatesBlocklistTest1::StaticClass(), "Begin", true },
		{ UMachineState_StatesBlocklistTest1::StaticClass(), "End", true },
		{ UMachineState_StatesBlocklistTest3::StaticClass(), "Begin", true },
		{ UMachineState_StatesBlocklistTest3::StaticClass(), "End", true },
		{ UMachineState_StatesBlocklistTest2::StaticClass(), "Begin", true },
		{ UMachineState_StatesBlocklistTest2::StaticClass(), "End", true },
		{ UMachineState_StatesBlocklistTest1::StaticClass(), "Begin", true },
		{ UMachineState_StatesBlocklistTest1::StaticClass(), "Popped", true },
	};

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	// Register states
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_StatesBlocklistTest1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_StatesBlocklistTest2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_StatesBlocklistTest3::StaticClass()));

	// Begin
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_StatesBlocklistTest1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_StatesBlocklistTest1::StaticClass()));

	// 1 blocks 2, so it should fail. Going to 3 should work
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState_Fail(this, &TestActor, UMachineState_StatesBlocklistTest2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_StatesBlocklistTest1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_StatesBlocklistTest3::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_StatesBlocklistTest3::StaticClass()));

	// 3 blocks 1, so it should fail. Going to 2 should work
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState_Fail(this, &TestActor, UMachineState_StatesBlocklistTest1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_StatesBlocklistTest3::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_StatesBlocklistTest2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_StatesBlocklistTest2::StaticClass()));

	// 2 blocks 3, so it should fail. Going to 1 should work
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState_Fail(this, &TestActor, UMachineState_StatesBlocklistTest3::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_StatesBlocklistTest2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_StatesBlocklistTest1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_StatesBlocklistTest1::StaticClass()));

	// Finish the test
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineExternalPushPopTest, "UE5FSM.ExternalPushPopTest",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineExternalPushPopTest::RunTest(const FString& Parameters)
{
	static TArray<FStateMachineTestMessage> ExpectedTestMessages;
	ExpectedTestMessages.Empty();

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	// Register states
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_ExternalPushPopTest1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_ExternalPushPopTest2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_ExternalPushPopTest3::StaticClass()));

	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_ExternalPushPopTest1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_ExternalPushPopTest1::StaticClass()));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest1, "Begin", true));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f)); // tick
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_ExternalPushPopTest2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_ExternalPushPopTest2::StaticClass()));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest1, "Paused", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest2, "Pushed", true));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f)); // tick
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_ExternalPushPopTest3::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_ExternalPushPopTest3::StaticClass()));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest2, "Paused", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest3, "Pushed", true));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest3, "Post default label", true));

	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, UMachineState_ExternalPushPopTest2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f)); // tick

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest3, "Popped", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest2, "Resumed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest2, "Post default label", true));

	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, UMachineState_ExternalPushPopTest1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f)); // tick

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest2, "Popped", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest1, "Resumed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest1, "Post default label", true));

	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_ExternalPushPopTest1, "Popped", true));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineStartWithNotByDefaultLabelTest, "UE5FSM.StartWithNotByDefaultLabelTest",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineStartWithNotByDefaultLabelTest::RunTest(const FString& Parameters)
{
	static TArray<FStateMachineTestMessage> ExpectedTestMessages;
	ExpectedTestMessages.Empty();

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_StartWithNotDefaultLabel::StaticClass()));

	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_StartWithNotDefaultLabel::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_StartWithNotDefaultLabel::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f)); // tick
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_StartWithNotDefaultLabel, "Begin", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_StartWithNotDefaultLabel, "Post test label", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_StartWithNotDefaultLabel, "Popped", true));

	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_StartWithNotDefaultLabel::StaticClass(), TAG_StateMachine_Label_Test));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_StartWithNotDefaultLabel::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f)); // tick
	ADD_LATENT_AUTOMATION_COMMAND(FPopState(this, &TestActor, nullptr));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_StartWithNotDefaultLabel, "Begin", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_StartWithNotDefaultLabel, "Post test label", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_StartWithNotDefaultLabel, "Popped", true));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineExternalPushPopTest2, "UE5FSM.ExternalPushPopTest2",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineExternalPushPopTest2::RunTest(const FString& Parameters)
{
	static const TArray<FStateMachineTestMessage> ExpectedTestMessages
	{
		{ UMachineState_ExternalPushTest1::StaticClass(), "Begin", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Prior to push", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Paused", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Pushed", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Prior to push", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Paused", true },
		{ UMachineState_ExternalPushTest3::StaticClass(), "Pushed", true },
		{ UMachineState_ExternalPushTest3::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest3::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest3::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest3::StaticClass(), "Prior to pop", true },
		{ UMachineState_ExternalPushTest3::StaticClass(), "Popped", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Resumed", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Prior to pop", true },
		{ UMachineState_ExternalPushTest2::StaticClass(), "Popped", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Resumed", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Hello", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Prior to pop", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "Popped", true },
		{ UMachineState_ExternalPushTest1::StaticClass(), "End test", true },
	};

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_ExternalPushTest1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_ExternalPushTest2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_ExternalPushTest3::StaticClass()));

	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_ExternalPushTest1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FIsInState(this, &TestActor, UMachineState_ExternalPushTest1::StaticClass()));

	// Wait until we're notified that the test has ended
	ADD_LATENT_AUTOMATION_COMMAND(FWaitPushPopMessage(this, "End test", 20.f, true));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FFiniteStateMachineBlockedPushTest_LatentImpl,
	FAutomationTestBase*, Test, AFiniteStateMachineTestActor**, TestActor);
bool FFiniteStateMachineBlockedPushTest_LatentImpl::Update()
{
	LATENT_TEST_BEGIN();
	StateMachine->RegisterState(UMachineState_BlockedPushTest1::StaticClass());
	StateMachine->RegisterState(UMachineState_BlockedPushTest2::StaticClass());
	StateMachine->RegisterState(UMachineState_BlockedPushTest3::StaticClass());
	StateMachine->RegisterState(UMachineState_BlockedPushTest4::StaticClass());
	StateMachine->RegisterState(UMachineState_BlockedPushTest5::StaticClass());

	StateMachine->GotoState(UMachineState_BlockedPushTest1::StaticClass());

	FFSM_PushRequestHandle Handle1;
	StateMachine->PushStateQueued(Handle1, UMachineState_BlockedPushTest2::StaticClass());
	LATENT_TEST_TRUE("Handle 1 is pending", Handle1.IsPending());
	Handle1.BindOnResultCallback(FOnPendingPushRequestSignature::FDelegate::CreateLambda(
		[] (EFSM_PendingPushRequestResult Result)
		{
			OnMessageDelegate.Broadcast( {
				UMachineState_BlockedPushTest1::StaticClass(),
				UEnum::GetValueAsString(Result),
				true
			});
		}));

	FFSM_PushRequestHandle Handle2;
	StateMachine->PushStateQueued(Handle2, UMachineState_BlockedPushTest3::StaticClass());
	LATENT_TEST_TRUE("Handle 2 is not pending", !Handle2.IsPending());

	FFSM_PushRequestHandle Handle3;
	StateMachine->PushStateQueued(Handle3, UMachineState_BlockedPushTest4::StaticClass());
	LATENT_TEST_TRUE("Handle 3 is pending", Handle3.IsPending());
	Handle3.BindOnResultCallback(FOnPendingPushRequestSignature::FDelegate::CreateLambda(
		[] (EFSM_PendingPushRequestResult Result)
		{
			OnMessageDelegate.Broadcast({
				UMachineState_BlockedPushTest4::StaticClass(),
				UEnum::GetValueAsString(Result),
			true
			});
		}));

	StateMachine->PopState();

	FFSM_PushRequestHandle Handle4;
	StateMachine->PushStateQueued(Handle4, UMachineState_BlockedPushTest5::StaticClass());
	LATENT_TEST_TRUE("Handle 4 is pending", Handle4.IsPending());
	Handle4.BindOnResultCallback(FOnPendingPushRequestSignature::FDelegate::CreateLambda(
		[] (EFSM_PendingPushRequestResult Result)
		{
			OnMessageDelegate.Broadcast({
				UMachineState_BlockedPushTest5::StaticClass(),
				UEnum::GetValueAsString(Result),
			true
			});
		}));

	StateMachine->PopState();
	StateMachine->PopState();

	FFSM_PushRequestHandle Handle5;
	StateMachine->PushStateQueued(Handle5, UMachineState_BlockedPushTest5::StaticClass());
	LATENT_TEST_TRUE("Handle 5 is pending", Handle5.IsPending());
	Handle5.BindOnResultCallback(FOnPendingPushRequestSignature::FDelegate::CreateLambda(
		[] (EFSM_PendingPushRequestResult Result)
		{
			OnMessageDelegate.Broadcast({
				UMachineState_BlockedPushTest5::StaticClass(),
				UEnum::GetValueAsString(Result),
			true
			});
		}));

	Handle5.Cancel();

	StateMachine->PopState();
	StateMachine->PopState();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineBlockedPushTest, "UE5FSM.BlockedPushTest",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineBlockedPushTest::RunTest(const FString& Parameters)
{
	static TArray<FStateMachineTestMessage> ExpectedTestMessages;
	ExpectedTestMessages.Empty();

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	ADD_LATENT_AUTOMATION_COMMAND(FFiniteStateMachineBlockedPushTest_LatentImpl(this, &TestActor));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest1, "Begin", true));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest1, "Paused", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest3, "Pushed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest1, "EFSM_PendingPushRequestResult::Success", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest3, "Paused", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest2, "Pushed", true));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest2, "Popped", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest3, "Resumed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest4, "EFSM_PendingPushRequestResult::Success", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest3, "Paused", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest4, "Pushed", true));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest4, "Popped", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest3, "Resumed", true));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest3, "Popped", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest1, "Resumed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest5, "EFSM_PendingPushRequestResult::Success", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest1, "Paused", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest5, "Pushed", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest5, "EFSM_PendingPushRequestResult::Canceled", true));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest5, "Popped", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest1, "Resumed", true));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_BlockedPushTest1, "Popped", true));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineLatentActionTest, "UE5FSM.LatentActionTest",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineLatentActionTest::RunTest(const FString& Parameters)
{
	static TArray<FStateMachineTestMessage> ExpectedTestMessages;
	ExpectedTestMessages.Empty();

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_LatentActions1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_LatentActions2::StaticClass()));

	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_LatentActions1::StaticClass(), TAG_StateMachine_Label_Default));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_LatentActions1, "Begin", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_LatentActions1, "End", true));
	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_LatentActions2, "Begin", true));

	ADD_LATENT_AUTOMATION_COMMAND(FEndState(this, &TestActor, nullptr));

	ExpectedTestMessages.Add(PREDICTED_TEST_MESSAGE(UMachineState_LatentActions2, "End", true));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFiniteStateMachineClearStackTest, "UE5FSM.ClearStackTest",
	EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::HighPriority |
	EAutomationTestFlags::ProductFilter);

bool FFiniteStateMachineClearStackTest::RunTest(const FString& Parameters)
{
	static const TArray<FStateMachineTestMessage> ExpectedTestMessages
	{
		{ UMachineState_Test1::StaticClass(), "Begin", true },
		{ UMachineState_Test1::StaticClass(), "Paused", true },
		{ UMachineState_Test2::StaticClass(), "Pushed", true },
		{ UMachineState_Test2::StaticClass(), "Paused", true },
		{ UMachineState_Test3::StaticClass(), "Pushed", true },
		{ UMachineState_Test3::StaticClass(), "End", true },
		{ UMachineState_Test2::StaticClass(), "Resumed", true },
		{ UMachineState_Test2::StaticClass(), "End", true },
		{ UMachineState_Test1::StaticClass(), "Resumed", true },
		{ UMachineState_Test1::StaticClass(), "End", true },
	};

	// Setup environment and test objects
	ADD_LATENT_AUTOMATION_COMMAND(FStartLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString("/Engine/Maps/Entry")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCreateTestActor(this, &TestActor));

	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_Test1::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_Test2::StaticClass()));
	ADD_LATENT_AUTOMATION_COMMAND(FRegisterState(this, &TestActor, UMachineState_Test3::StaticClass()));

	ADD_LATENT_AUTOMATION_COMMAND(FGotoState(this, &TestActor, UMachineState_Test1::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test2::StaticClass(), TAG_StateMachine_Label_Default));
	ADD_LATENT_AUTOMATION_COMMAND(FPushState(this, &TestActor, UMachineState_Test3::StaticClass(), TAG_StateMachine_Label_Default));

	ADD_LATENT_AUTOMATION_COMMAND(FClearStack(this, &TestActor, 3));

	// Check whether all predicted events took place in the correct order from the correct states
	ADD_LATENT_AUTOMATION_COMMAND(FCompareTestMessages(this, ExpectedTestMessages));

	// Finish test
	ADD_LATENT_AUTOMATION_COMMAND(FEndLatentTest());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

	return true;
}
