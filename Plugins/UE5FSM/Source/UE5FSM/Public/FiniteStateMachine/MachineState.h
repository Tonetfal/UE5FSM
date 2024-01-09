#pragma once

#include "FiniteStateMachine/FiniteStateMachineTypes.h"
#include "FiniteStateMachine/GlobalMachineStateInterface.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "UE5Coro.h"
#include "UE5CoroAI.h"
#include "UObject/Object.h"

#include "MachineState.generated.h"

class UFiniteStateMachine;
class UMachineStateData;

using namespace UE5Coro;

/** Label tag associated with the default label the states start with if not told otherwise. */
UE5FSM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateMachine_Label_Default);

/**
 * Utility macro used to easily register labels that 100% match the naming.
 *
 * The label tag must be called TAG_StateMachine_Label_*YOUR LABEL NAME*,
 * while the function must be called Label_*YOUR LABEL NAME*.
 *
 * It's encouraged to follow this naming for tags and functions, as it makes code clear and consistant.
 */
#define REGISTER_LABEL(LABEL_NAME) RegisterLabel( \
	TAG_StateMachine_Label_ ## LABEL_NAME, FLabelSignature::CreateUObject(this, &ThisClass::Label_ ## LABEL_NAME))

/**
 * All the GOTO_STATE, GOTO_LABEL, PUSH_STATE and POP_STATE with any variation must be used only inside the labels.
 * Outside them use the normal functions, as the macro serve as utilities to avoid mistakes in labels, such as missing
 * co_return and things like that.
 */

#define GOTO_STATE_IMPLEMENTATION(STATE_CLASS, LABEL, ...) \
	if (GotoState(STATE_CLASS, LABEL, ## __VA_ARGS__)) co_return

#define GOTO_STATE(STATE_NAME) \
	GOTO_STATE_CLASS(STATE_NAME ## ::StaticClass())

#define GOTO_STATE_CLASS(STATE_CLASS) \
	GOTO_STATE_IMPLEMENTATION(STATE_CLASS, TAG_StateMachine_Label_Default)

#define GOTO_STATE_LABEL(STATE_NAME, LABEL_NAME, ...) \
	GOTO_STATE_CLASS_LABEL(STATE_NAME ## ::StaticClass(), LABEL_NAME, ## __VA_ARGS__)

#define GOTO_STATE_CLASS_LABEL(STATE_CLASS, LABEL_NAME, ...) \
	GOTO_STATE_IMPLEMENTATION(STATE_CLASS, TAG_StateMachine_Label_ ## LABEL_NAME, ## __VA_ARGS__)

#define GOTO_LABEL(LABEL_NAME) if (GotoLabel(TAG_StateMachine_Label_ ## LABEL_NAME)) co_return

#define PUSH_STATE_IMPLEMENTATION(STATE_CLASS, LABEL) \
	RUN_LATENT_EXECUTION(PushState, STATE_CLASS, LABEL)

#define PUSH_STATE(STATE_NAME) \
	PUSH_STATE_LABEL(STATE_NAME, Default)

#define PUSH_STATE_LABEL(STATE_NAME, LABEL_NAME) \
	PUSH_STATE_IMPLEMENTATION(STATE_NAME ## ::StaticClass(), TAG_StateMachine_Label_ ## LABEL_NAME)

#define PUSH_STATE_CLASS(STATE_CLASS) \
	PUSH_STATE_CLASS_LABEL(STATE_CLASS, Default)

#define PUSH_STATE_CLASS_LABEL(STATE_CLASS, LABEL_NAME) \
	PUSH_STATE_IMPLEMENTATION(STATE_CLASS, TAG_StateMachine_Label_ ## LABEL_NAME)

#define POP_STATE() if (PopState()) co_return

#define TO_STR(x) #x
#define RUN_LATENT_EXECUTION(FUNCTION, ...) \
	RunLatentExecutionExt(LIFT(FUNCTION), *FString::Printf(TEXT("Caller function [%s] Line [%d] Latent function [%s]"), \
		*FString(__FUNCTION__), __LINE__, *FString(TO_STR(FUNCTION))), ## __VA_ARGS__)

/**
 * Utility macro to use to pass functions with templated parameters or defaulted parameters you're not willing to change.
 *
 * Example:
 * - LatentExecution(LIFT(AI::AIMoveTo), Controller, Vector);
 * - LatentExecution(LIFT(AI::AIMoveTo), Controller, Actor);
 */
#define LIFT(FUNCTION) [this] <typename... TArgs> (TArgs&&... Args) { return FUNCTION(Forward<TArgs>(Args)...); }

/**
 * Available actions the state can perform.
 */
UENUM()
enum class EStateAction : uint8
{
	None,
	Begin,
	End,
	Push,
	Pop,
	Resume,
	Pause
};

/**
 * Finite machine's state defining some behavior of an object. <br> <br>
 *
 * # Functions to look at when implementing new state: Begin, Tick, End, and Label_Default.
 * - Begin: Function called when state starts.
 * - End: Function called when state terminates.
 * - Tick: Function called each tick.
 * - Label_Default: Label function that is called when state starts by default once.
 *
 * # Functions to add and change the behavior: REGISTER_LABEL, GotoState, GotoLabel, and StopLatentExecution.
 * - REGISTER_LABEL: macro to register a label. Read the macro documentation for more detailed information.
 * - GotoState: Change current state the finite state machine is in, which causes End to be called.
 * - GotoLabel: Start executing a given label.
 * - StopLatentExecution: cancel all latent functions (coroutines) activated by this label. Note that anything latent
 * must be cleaned yourself, as nothing outside the state knows about the latent execution of a state.
 *
 * # Labels
 * - Labels are special functions coroutines can be used in, making them a good place to create latent gameplay code.
 * - To distinguish them from normal functions use "Label_" as a prefix.
 * - They are like mini states within a single state, but when a label finishes executing its code, nothing happens
 * automatically afterwards. Labels either have to manage themselves, so that they carry on the logic invokation, or
 * something else, like state functions, have to do it instead; it depends on the context.
 *
 * # State data
 * - State data is data that a particular state contains, and that is accessible from outside using the owner state
 * machine, to make it easy to expose information other states might need to base their behavior on, or pass some
 * information to us.
 * - The data object is created once on state registration, and stays there until the end of the state life cycle.
 * - To define the subclass of the data object you want to use for a particular state use UMachineState::StateDataClass.
 */
UCLASS(Abstract, Blueprintable, ClassGroup=("Finite State Machine"))
class UE5FSM_API UMachineState
	: public UObject
{
	GENERATED_BODY()

public:
	/** Has to initialize the state. */
	friend UFiniteStateMachine;

public:
	DECLARE_DELEGATE_RetVal(
		TCoroutine<>, FLabelSignature);

	DECLARE_MULTICAST_DELEGATE_OneParam(
		FOnStateActionSignature,
		EStateAction StateAction);

public:
	UMachineState();
	virtual ~UMachineState() override;

	/**
	 * Check whether the state is valid or not.
	 * @return 	If true, the state has been or being destroyed, false otherwise.
	 */
	bool IsStateValid() const;

	/**
	 * Get currently active label.
	 * @return	Label tag that is currently active.
	 */
	FGameplayTag GetActiveLabel() const;

	/**
	 * Get last state action that took place.
	 * @return	Last state action.
	 */
	EStateAction GetLastStateAction() const;

	/**
	 * Get time since last state action took place.
	 * @return	Time since last state action took place.
	 */
	float GetTimeSinceLastStateAction() const;

	/**
	 * Get debug string. It will be used by the gameplay debugger category for UE5FSM.
	 *
	 * In here you can form a string describing important fields you might need to debug your machine states, for
	 * instance, in case of enemy AI character, the target actor/location, or any other relevant data.
	 *
	 * If empty, this state won't be present in the extended debug information.
	 *
	 * @return	Debug string.
	 */
	virtual FString GetDebugData() const;

protected:
	/**
	 * Called when state starts the execution.
	 */
	virtual void Begin(TSubclassOf<UMachineState> PreviousState);

	/**
	 * Called when state terminates the execution.
	 */
	virtual void End(TSubclassOf<UMachineState> NewState);

	/**
	 * Called when state gets pushed to the state stack.
	 * @note	When state is pushed, Begin is not called.
	 */
	virtual void Pushed();

	/**
	 * Called when state gets pushed to the state stack.
	 * @note	When state is pushed, Begin is not called.
	 */
	virtual void Popped();

	/**
	 * Called when another state got pushed when we were the top-most one.
	 */
	virtual void Paused();

	/**
	 * Called when another state got popped from the stack, leaving us be the top-most one.
	 */
	virtual void Resumed();

	/**
	 * Called when the state either begins or get pushed to the stack.
	 * @see		UMachineState::Begin, UMachineState::Pushed
	 */
	virtual void InitState();

	/**
	 * Called when the state either ends or get popped from the stack.
	 * @see		UMachineState::End, UMachineState::Popped
	 */
	virtual void ClearState();

	/**
	 * Register a new label this state contains.
	 * @param	Label gameplay tag associated with the label.
	 * @param	Callback function to call when a label is activated.
	 * @see		REGISTER_LABEL()
	 */
	bool RegisterLabel(FGameplayTag Label, const FLabelSignature& Callback);

	/**
	 * Stop any latent execution of EVERY state known to the owning state machine. Doesn't interrupt label execution.
	 * @return	Amount of latent executions stopped.
	 *
	 * @note	If a latent execution is terminated while the state is paused, it's not going to carry on the execution
	 * unlles the state becomes active.
	 * @see		UMachineState::LatentExecution()
	 */
	int32 StopLatentExecution();

	/**
	 * Called after StopLatentExecution finishes executing.
	 *
	 * It's the place to put your custom code when latent execution is aborted.
	 */
	virtual void StopLatentExecution_Custom();

private:
	/**
	 * Stop any latent execution of this state. Doesn't interrupt label execution.
	 * @return	Amount of latent executions stopped.
	 *
	 * @note	If a latent execution is terminated while the state is paused, it's not going to carry on the execution
	 * unlles the state becomes active.
	 * @see		UMachineState::LatentExecution()
	 */
	int32 StopLatentExecution_Implementation();

	/**
	 * Stop all latent functions (coroutines) in control of this state.
	 * @return	Amount of latent executions stopped.
	 */
	int32 StopRunningLabels();

	/**
	 * Remove all latent execution cancel delegates that are no longer bound because the latent execution the delegate
	 * was associated with has terminated before the user has used it.
	 * @return	Amount of removed invalid latent execution cancellers.
	 */
	int32 ClearInvalidLatentExecutionCancellers();

protected:
	/**
	 * Check if this state is the active one.
	 * @return	True if state is active, false otherwise.
	 */
	bool IsStateActive() const;

#pragma region UFiniteStateMachine Contract

protected:
	/**
	 * Called each frame.
	 * @param	DeltaSeconds time since last tick.
	 */
	virtual void Tick(float DeltaSeconds);

	/**
	 * Initialize the state.
	 */
	virtual void Initialize();

	/**
	 * Create persistant state data.
	 */
	virtual UMachineStateData* CreateStateData();

private:
	/**
	 * Set state machine that manages this state.
	 */
	void SetStateMachine(UFiniteStateMachine* InStateMachine);

	/**
	 * Set initial label the state starts with.
	 */
	void SetInitialLabel(FGameplayTag Label);

	/**
	 * Called when state action takes place.
	 * @param	StateAction state action to trigger.
	 * @param	OptionalData optional data client might send.
	 */
	void OnStateAction(EStateAction StateAction, void* OptionalData = nullptr);

	/**
	 * Check whether this state can safely be deactivated.
	 * @return	True if it can, false otherwise.
	 */
	bool CanSafelyDeactivate(FString& OutReason) const;
#pragma endregion

#pragma region Labels

protected:
	/**
	 * Default label the state starts with, if not told otherwise.
	 */
	virtual TCoroutine<> Label_Default();
#pragma endregion

	/**
	 * Default function to use to run any latent execution from labels.
	 *
	 * Example:
	 * - co_await RunLatentExecution(Latent::NextTick);
	 * - co_await RunLatentExecution(Latent::Seconds, 5.0);
	 * - co_await RunLatentExecution(LIFT(AI::AIMoveTo), Controller, Actor);
	 *
	 * @param	Function latent function to execute. Result must be co_awaitable.
	 * @param	Args arguments to pass.
	 * @see		LIFT()
	 */
	template<typename TFunction, typename... TArgs>
	TCoroutine<> RunLatentExecution(TFunction Function, TArgs&&... Args);

	template<typename TFunction, typename... TArgs>
	TCoroutine<> RunLatentExecutionExt(TFunction Function, FString DebugInfo, TArgs&&... Args);

private:
	FString GetDebugString(const FString& RunLatentExecutionExt) const;

public:
	/**
	 * Go to a requested state at a requested label.
	 * @param	InStateClass state to go to.
	 * @param	Label label to start the state with.
	 * @return	If true, state has been successfully switched, false otherwise.
	 * @param	bForceEvents in case of switching to the same state we're in: If true, fire end & begin events,
	 * otherwise do not.
	 * @note	Unlike Unreal 3, when succeeds, it doesn't interrupt latent code execution the function is called
	 * from (if any). It is the caller obligation to call co_return (or simply not have any code after a successful
	 * GotoState).
	 */
	bool GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default,
		bool bForceEvents = true);

	/**
	 * Go to a requested label.
	 * @param	Label label to go to.
	 * @return	If true, label has been activated, false otherwise.
	 */
	bool GotoLabel(FGameplayTag Label);

	/**
	 * Push a specified state at a requested label on top of the stack.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state with.
	 * @param	bOutPrematureResult output parameter. Boolean to use when you want to use the function result before it
	 * returns code execution.
	 */
	TCoroutine<> PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = FGameplayTag::EmptyTag,
		bool* bOutPrematureResult = nullptr);

	/**
	 * Pop the top-most state from stack.
	 * @return	If true, a state has been popped, false otherwise.
	 * @note	Unlike Unreal 3, when succeeds, it doesn't interrupt latent code execution the function is called
	 * from (if any). It is the caller obligation to call co_return (or simply not have any code after a successful
	 * GotoState).
	 */
	bool PopState();

	/**
	 * Check whether state contains a given label.
	 * @param	Label label to check the presence of.
	 * @return	If true, label is contained in this state, false otherwise.
	 */
	bool ContainsLabel(FGameplayTag Label) const;

	/**
	 * Check whether a given tag is valid for a label.
	 * @param	Tag tag to check for validness.
	 * @return	If true, label is valid, false otherwise.
	 */
	static bool IsLabelTagCorrect(FGameplayTag Tag);

#pragma region Utilities

protected:
	/**
	 * Get typed owner.
	 * @return Typed owner.
	 */
	template<typename T>
	T* GetOwner() const;

	/**
	 * Get typed owner checked.
	 * @return Typed owner checked.
	 */
	template<typename T>
	T* GetOwnerChecked() const;

	/**
	 * Get current game time in seconds.
	 * @return	Game time in seconds.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="Get Time"))
	float GetTime() const;

	/**
	 * Get game time since the given amount in seconds.
	 * @return	Time since the given time.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="Time Since"))
	float TimeSince(float Time) const;

	/**
	 * Get world timer manager.
	 * @return	Timer manager.
	 */
	FTimerManager& GetTimerManager();
#pragma endregion

public:
	/** Fired when state action has been performed. */
	FOnStateActionSignature OnStateActionDelegate;

private:
	struct FLatentExecution
	{
	public:
		FSimpleDelegate CancelDelegate;
		FString DebugData;
	};

protected:
	/** Class defining state data object to create to manage data of this state. */
	UPROPERTY(EditDefaultsOnly, Category="Data")
	TSubclassOf<UMachineStateData> StateDataClass = nullptr;

	/** Machine state classes that cannot be activated using GotoState while this one is the active one. */
	UPROPERTY(EditDefaultsOnly, Category="State Transition")
	TArray<TSubclassOf<UMachineState>> StatesBlocklist;

	/** Reference to the base state data object. It's intended to be downcasted to get the subclasses version. */
	UPROPERTY()
	TObjectPtr<UMachineStateData> BaseStateData = nullptr;

	/** Finite state machine we're managed by. */
	TWeakObjectPtr<UFiniteStateMachine> StateMachine = nullptr;

	/** Label we are currently in. */
	FGameplayTag ActiveLabel = TAG_StateMachine_Label_Default;

private:
	/** All registered labels. */
	TMap<FGameplayTag, FLabelSignature> RegisteredLabels;

	/** If true, UMachineState::ActiveLabel has been activated, false otherwise. */
	bool bLabelActivated = false;

	/** Handles associated with the coroutines used by this state. */
	TArray<TPair<TCoroutine<>, FString>> RunningLabels;

	/**
	 * If true, active label is being activated, false otherwise.
	 * It prevents users from activating labels while some other is being activated.
	 */
	bool bIsActivatingLabel = false;

	/** Fired when user wants to cancel all non-label latent executions, such as Sleep, AIMoveTo and others. */
	TArray<FLatentExecution> RunningLatentExecutions;

	/** If true, the object has been destroyed, false otherwise. */
	bool bIsDestroyed = false;

	/** Last state action that took place. Is used for debug purposes. */
	EStateAction LastStateAction = EStateAction::None;

	/** Time the last state action took place. */
	float LastStateActionTime = 0.f;
};

template<typename TFunction, typename... TArgs>
TCoroutine<> UMachineState::RunLatentExecution(TFunction Function, TArgs&&... Args)
{
	return RunLatentExecutionExt(Function, "", Forward<TArgs>(Args)...);
}

template<typename TFunction, typename ... TArgs>
TCoroutine<> UMachineState::RunLatentExecutionExt(TFunction Function, FString DebugInfo, TArgs&&... Args)
{
	// Wrap this coroutine in a custom way to support custom cancellation
	FLatentExecution& LatentExecutionWrapper = RunningLatentExecutions.AddDefaulted_GetRef();

	// Save debug data
	LatentExecutionWrapper.DebugData = DebugInfo;

#ifdef FSM_EXTREME_VERBOSITY
	UE_LOG(LogFiniteStateMachine, VeryVerbose, TEXT("%s"), *GetDebugString(LatentExecutionWrapper.DebugData));
#endif

	// Allow user to cancel an awaiter
	auto Cancelling = Latent::UntilDelegate(LatentExecutionWrapper.CancelDelegate);

	// Create the asked awaiter
	auto LatentExecution = Function(Forward<TArgs>(Args)...);

	// Wait until either the latent execution terminates or we're explicitly cancelled
	co_await WhenAny(MoveTemp(LatentExecution), MoveTemp(Cancelling));

	// Wait until the state becomes active (if not already) or invalid
	co_await Latent::Until([this]
	{
		if (!IsStateValid())
		{
			return true;
		}

		return IsStateActive();
	});

	co_await FinishNowIfCanceled();
}

template<typename T>
T* UMachineState::GetOwner() const
{
	auto* Outer = GetOuter();
	if (!IsValid(Outer))
	{
		return nullptr;
	}

	auto* Owner = Cast<T>(Outer);
	if (!IsValid(Owner))
	{
		return nullptr;
	}

	return Owner;
}

template<typename T>
T* UMachineState::GetOwnerChecked() const
{
	auto* Owner = GetOwner<T>();
	check(IsValid(Owner));

	return Owner;
}
