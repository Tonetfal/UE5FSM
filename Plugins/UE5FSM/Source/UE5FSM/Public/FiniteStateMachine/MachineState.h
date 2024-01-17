#pragma once

#include "FiniteStateMachine/FiniteStateMachineTypes.h"
#include "FiniteStateMachine/GlobalMachineStateInterface.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "UE5Coro.h"
#include "UE5CoroAI.h"

#include "MachineState.generated.h"

class UFiniteStateMachine;
class UMachineStateData;
struct FFSM_PushRequestHandle;
struct FMS_IsDispatchingEventManager;

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

#define PUSH_STATE_IMPLEMENTATION(STATE_CLASS, LABEL) \
	RUN_LATENT_EXECUTION(PushState, STATE_CLASS, LABEL)

#define PUSH_STATE(STATE_NAME) \
	PUSH_STATE_LABEL(STATE_NAME, Default)

#define PUSH_STATE_CLASS(STATE_CLASS) \
	PUSH_STATE_CLASS_LABEL(STATE_CLASS, Default)

#define PUSH_STATE_LABEL(STATE_NAME, LABEL_NAME) \
	PUSH_STATE_IMPLEMENTATION(STATE_NAME ## ::StaticClass(), TAG_StateMachine_Label_ ## LABEL_NAME)

#define PUSH_STATE_CLASS_LABEL(STATE_CLASS, LABEL_NAME) \
	PUSH_STATE_IMPLEMENTATION(STATE_CLASS, TAG_StateMachine_Label_ ## LABEL_NAME)

#define PUSH_STATE_QUEUED_IMPLEMENTATION(HANDLE, STATE_CLASS, LABEL) \
	RUN_LATENT_EXECUTION(PushStateQueued, HANDLE, STATE_CLASS, LABEL)

#define PUSH_STATE_QUEUED(HANDLE, STATE_NAME) \
	PUSH_STATE_QUEUED_LABEL(HANDLE, STATE_NAME, Default)

#define PUSH_STATE_QUEUED_CLASS(HANDLE, STATE_CLASS) \
	PUSH_STATE_QUEUED_CLASS_LABEL(HANDLE, STATE_CLASS, Default)

#define PUSH_STATE_QUEUED_LABEL(HANDLE, STATE_NAME, LABEL_NAME) \
	PUSH_STATE_QUEUED_IMPLEMENTATION(HANDLE, STATE_NAME ## ::StaticClass(), TAG_StateMachine_Label_ ## LABEL_NAME)

#define PUSH_STATE_QUEUED_CLASS_LABEL(HANDLE, STATE_CLASS, LABEL_NAME) \
	PUSH_STATE_QUEUED_IMPLEMENTATION(HANDLE, STATE_CLASS, TAG_StateMachine_Label_ ## LABEL_NAME)

#define GOTO_LABEL(LABEL_NAME) do { if (GotoLabel(TAG_StateMachine_Label_ ## LABEL_NAME)) co_return; } while(0)
#define POP_STATE() do { if (PopState()) co_return; } while(0)
#define END_STATE() do { if (EndState()) co_return; } while(0)
#define CLEAR_STACK() do { ClearStack(); co_return; } while(0)

#define TO_STR(x) #x
#define RUN_LATENT_EXECUTION(FUNCTION, ...) \
	co_await RunLatentExecutionExt(LIFT(FUNCTION), *FString::Printf( \
		TEXT("Caller function [%s] Line [%d] Latent function [%s]"), \
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
 * Finite machine's state defining behavior of an object. <br> <br>
 *
 * # Functions to add and change state behavior: REGISTER_LABEL(), GOTO_STATE(), GOTO_LABEL(), and StopLatentExecution.
 * - REGISTER_LABEL(): macro to register a label. Read the macro documentation for more detailed information.
 * - GOTO_STATE(): macro to go to another state. Change current state the finite state machine is in.
 * - GOTO_LABEL(): macro to go to another label. Start executing a given label.
 * - StopLatentExecution: cancel all latent functions (coroutines) running under owning state machine.
 *
 * # Labels
 * - Labels are special functions coroutines are meant to be used in, making them a good place to create latent gameplay
 *   code.
 * - To distinguish them from normal functions use "Label_" as a prefix.
 * - They are like mini-states within a single state, but, when a label finishes executing its code, nothing happens
 * automatically afterwards. Labels either have to manage themselves, so that they carry on the logic invocation, or
 * something else, like state functions, has to do it instead; it depends on the context.
 *
 * # State data
 * - State data is an object that a particular state has, and it's accessible from outside using the owning state
 * machine to make it easy to expose information other states might need to base their behavior on, or pass some
 * information to this state.
 * - The object is created once on state registration, and destroyes at the end of the state lifecycle.
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

	/** It queues certain users calls (GotoState, PushState, PopState) while dispatching event. */
	friend FMS_IsDispatchingEventManager;

public:
	DECLARE_DELEGATE_RetVal(
		UE5Coro::TCoroutine<>, FLabelSignature);

	DECLARE_MULTICAST_DELEGATE_TwoParams(
		FOnStateActionSignature,
		UMachineState* State,
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
	 * Check whether this machine state is currently dispatching an event (Begin, End, Push, Pop, Resume, or Pop).
	 * @return 	If true, an event is being dispatch, false otherwise.
	 */
	bool IsDispatchingEvent() const;

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

#pragma region Events
protected:
	/**
	 * Called when state starts the execution.
	 */
	virtual void OnBegan(TSubclassOf<UMachineState> OldState);

	/**
	 * Called when state terminates the execution.
	 */
	virtual void OnEnded(TSubclassOf<UMachineState> NewState);

	/**
	 * Called when state gets pushed to the state stack.
	 * @note	When state is pushed, Begin is not called.
	 */
	virtual void OnPushed(TSubclassOf<UMachineState> OldState);

	/**
	 * Called when state gets pushed to the state stack.
	 * @note	When state is pushed, Begin is not called.
	 */
	virtual void OnPopped(TSubclassOf<UMachineState> NewState);

	/**
	 * Called when another state got popped from the stack, leaving us be the top-most one.
	 */
	virtual void OnResumed(TSubclassOf<UMachineState> OldState);

	/**
	 * Called when another state got pushed when we were the top-most one.
	 */
	virtual void OnPaused(TSubclassOf<UMachineState> NewState);

	/**
	 * Called when the state becomes active, or, in other words, it becomes the top-most state on the stack.
	 * It's either just started or got resumed.
	 * @param	StateAction state action this function was called due.
	 * @param	OldState state that was active before this one was activated.
	 * @see		UMachineState::OnBegan, UMachineState::OnPushed, UMachineState::OnResumed
	 */
	virtual void OnActivated(EStateAction StateAction, TSubclassOf<UMachineState> OldState);

	/**
	 * Called when the state becomes inactive, or, in other words, it's not the top-most state on the stack anymore,
	 * yet it's present. It's either not present on the stack anymore or got paused.
	 * @param	StateAction state action this function was called due.
	 * @param	NewState state that got activated after this one got deactivated.
	 * @see		UMachineState::OnEnded, UMachineState::OnPopped, UMachineState::OnPaused
	 */
	virtual void OnDeactivated(EStateAction StateAction, TSubclassOf<UMachineState> NewState);

	/**
	 * Called when the state is added to the stack, i.e. it either began or got pushed on it.
	 * @param	StateAction state action this function was called due.
	 * @param	OldState state that this one was added upon.
	 * @see		UMachineState::OnBegan, UMachineState::OnPushed
	 */
	virtual void OnAddedToStack(EStateAction StateAction, TSubclassOf<UMachineState> OldState);

	/**
	 * Called when the state is not present on the stack anymore, i.e. it either ended or got popped out of it.
	 * @param	StateAction state action this function was called due.
	 * @param	NewState state that got activated after this one got deactivated.
	 * @see		UMachineState::OnEnded, UMachineState::Popped
	 */
	virtual void OnRemovedFromStack(EStateAction StateAction, TSubclassOf<UMachineState> NewState);
#pragma endregion

protected:
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
	 *
	 * @param	StoppedCoroutines number of coroutines that we've been executing, but not anymore.
	 */
	virtual void StopLatentExecution_Custom(int32 StoppedCoroutines);

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
	 * @param	StateClass state that is either the new active one, or the one that got deactivated.
	 */
	void OnStateAction(EStateAction StateAction, TSubclassOf<UMachineState> StateClass);

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
	virtual UE5Coro::TCoroutine<> Label_Default();
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
	UE5Coro::TCoroutine<> RunLatentExecution(TFunction Function, TArgs&&... Args);

	template<typename TFunction, typename... TArgs>
	UE5Coro::TCoroutine<> RunLatentExecutionExt(TFunction Function, FString DebugInfo, TArgs&&... Args);

private:
	FString GetDebugString(const FString& RunLatentExecutionExt) const;

public:
	/**
     * Activate a state at a specified label. If there's any active state, it'll deactivated.
	 * @param	InStateClass state to go to.
	 * @param	Label label to start the state with.
	 * @param	bForceEvents in case of switching to the same state we're in: If true, fire end & begin events,
	 * otherwise do not.
	 * @return	If true, state has been successfully switched, false otherwise.
	 * @note	Unlike Unreal 3, when succeeds, it doesn't interrupt latent code execution the function is called
	 * from (if any). It is the caller obligation to call co_return (or simply not have any code after a successful
	 * GotoState).
	 */
	bool GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default,
		bool bForceEvents = true);

	/**
	 * End the active state. If there's any state below this in the stack, it'll resume its execution.
	 * @return	If true, a state has ended, false otherwise.
	 * @note	Unlike Unreal 3, when succeeds, it doesn't interrupt latent code execution the function is called
	 * from (if any). It is the caller obligation to call co_return (or simply not have any code after a successful
	 * EndState).
	 */
	bool EndState();

	/**
	 * Go to a label using the active state.
	 * @param	Label label to go to.
	 * @return	If true, label has been activated, false otherwise.
	 */
	bool GotoLabel(FGameplayTag Label);

	/**
	 * Push a state at a specified label on top of the stack. If there's any active state, it'll be paused upon
	 * successful push.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state with.
	 * @param	bOutPrematureResult output parameter. Boolean to use when you want to use the function result before it
	 * returns code execution order.
	 */
	UE5Coro::TCoroutine<> PushState(TSubclassOf<UMachineState> InStateClass,
		FGameplayTag Label = TAG_StateMachine_Label_Default, bool* bOutPrematureResult = nullptr);

	/**
	 * Push a state at a specified label on top of the stack. If the operation is not possible to execute for
	 * any reason that might change in the future, it'll queued, and apply it as soon as it becomes possible following
	 * the existing queue. If there's any active state, it'll be paused upon successful push.
	 * @param	OutHandle output parameter. Push request handle used to interact with the request.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state at.
	 */
	UE5Coro::TCoroutine<> PushStateQueued(FFSM_PushRequestHandle& OutHandle,
		TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default);

	/**
	 * Pop the top-most state from stack. If there's any state below this in the stack, it'll resume its execution.
	 * @return	If true, a state has been popped, false otherwise.
	 * @note	Unlike Unreal 3, when succeeds, it doesn't interrupt latent code execution the function is called
	 * from (if any). It is the caller obligation to call co_return (or simply not have any code after a successful
	 * GotoState).
	 */
	bool PopState();

	/**
	 * Clear all states from the stack leaving it empty.
	 * @return	Amount of ended states.
	 */
	int32 ClearStack();

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

public:
	/**
	 * Get owner.
	 * @return Owner.
	 */
	AActor* GetOwner() const;

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

protected:
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
	FTimerManager& GetTimerManager() const;
#pragma endregion

public:
	/** Fired when state action has been performed. */
	FOnStateActionSignature OnStateActionDelegate;

private:
	/** Delegate to execute when an event has dispatched. It's intended to be used only by the FSM. */
	FSimpleDelegate OnFinishedDispatchingEvent;

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
	TArray<TPair<UE5Coro::TCoroutine<>, FString>> RunningLabels;

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

	/** If true, an event is currently dispatching, false otherwise. */
	bool bIsDispatchingEvent = false;
};

template<typename TFunction, typename... TArgs>
UE5Coro::TCoroutine<> UMachineState::RunLatentExecution(TFunction Function, TArgs&&... Args)
{
	return RunLatentExecutionExt(Function, "", Forward<TArgs>(Args)...);
}

template<typename TFunction, typename ... TArgs>
UE5Coro::TCoroutine<> UMachineState::RunLatentExecutionExt(TFunction Function, FString DebugInfo, TArgs&&... Args)
{
	// Wrap this coroutine in a custom way to support custom cancellation
	FLatentExecution& LatentExecutionWrapper = RunningLatentExecutions.AddDefaulted_GetRef();

	// Save debug data
	LatentExecutionWrapper.DebugData = DebugInfo;

#ifdef FSM_EXTREME_VERBOSITY
	UE_LOG(LogFiniteStateMachine, VeryVerbose, TEXT("%s"), *GetDebugString(LatentExecutionWrapper.DebugData));
#endif

	// Allow user to cancel an awaiter
	auto Cancelling = UE5Coro::Latent::UntilDelegate(LatentExecutionWrapper.CancelDelegate);

	// Create the asked awaiter
	auto LatentExecution = Function(Forward<TArgs>(Args)...);

	// Wait until either the latent execution terminates or we're explicitly cancelled
	co_await UE5Coro::WhenAny(MoveTemp(LatentExecution), MoveTemp(Cancelling));

	// Wait until the state becomes active (if not already) or invalid
	co_await UE5Coro::Latent::Until([this]
	{
		if (!IsStateValid())
		{
			return true;
		}

		return IsStateActive();
	});

	co_await UE5Coro::FinishNowIfCanceled();
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
