#pragma once

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
FINITESTATEMACHINEMODULE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateMachine_Label_Default);

/**
 * Utility macro used to easily register labels that 100% match the naming.
 *
 * The label tag must be called TAG_StateMachine_Label_*YOUR LABEL NAME*,
 * while the function must be called Label_*YOUR LABEL NAME*.
 *
 * It's encouraged to follow this naming for tags and functions, as it makes code clear and consistant.
 */
#define REGISTER_LABEL(LABEL_NAME) \
	RegisterLabel(TAG_StateMachine_Label_ ## LABEL_NAME, \
		FLabelSignature::CreateUObject(this, &ThisClass::Label_ ## LABEL_NAME));

/**
 * Utility macro to wrap GotoLabel call, making it easier to type it.
 */
#define GOTO_LABEL(LABEL) GotoLabel(TAG_StateMachine_Label_ ## LABEL)

/**
 * Utility macro to wrap GotoState call, making it easier to type it.
 */
#define GOTO_STATE(STATE) GotoState(UMTD_FoeState_ ## STATE ## ::StaticClass())

/**
 * Utility macro to wrap GotoState call with a label, making it easier to type it.
 * Also supports a third parameter for bForceEvents. Must be a boolean.
 */
#define GOTO_STATE_LABEL(STATE, LABEL, ...) \
	GotoState(UMTD_FoeState_ ## STATE ## ::StaticClass(), TAG_StateMachine_Label_ ## LABEL, ## __VA_ARGS__)

/**
 * Available actions the state can perform.
 */
enum class EStateAction : uint8
{
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
class FINITESTATEMACHINEMODULE_API UMachineState
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

#pragma region UFSM_StateMachine_Contract
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

public:
	/**
	 * Go to a requested state at a requested label.
	 * @param	InStateClass state to go to.
	 * @param	Label label to start the state with.
	 * @return	If true, state has been successfully switched, false otherwise.
	 * @note	Unlike Unreal 3, when succeeds, it doesn't interrupt latent code execution the function is called
	 * from (if any). It is the caller obligation to call co_return (or simply not have any code after a successful
	 * GotoState).
	 */
	bool GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default);

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
	 */
	TCoroutine<> PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = FGameplayTag::EmptyTag);

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
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="Get time"))
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

protected:
	/** Class defining state data object to create to manage data of this state. */
	UPROPERTY(EditDefaultsOnly, Category="Data")
	TSubclassOf<UMachineStateData> StateDataClass = nullptr;

	/** Reference to the base state data object. It's intended to be up-casted to get the subclasses version. */
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
	TArray<FAsyncCoroutine> RunningLabelCoroutines;

	/** Fired when user wants to cancel all non-label latent executions, such as Sleep, AIMoveTo and others. */
	TArray<FSimpleDelegate> RunningLatentExecutions;
};

template<typename TFunction, typename ... TArgs>
TCoroutine<> UMachineState::RunLatentExecution(TFunction Function, TArgs&&... Args)
{
	// Allow user to cancel an awaiter
	FSimpleDelegate& CancelDelegate = RunningLatentExecutions.AddDefaulted_GetRef();
	auto Cancelling = Latent::UntilDelegate(CancelDelegate);

	// Create the asked awaiter
	auto LatentExecution = Function(Forward<TArgs>(Args)...);

	// Wait until either the latent execution terminates or we're explicitly cancelled
	co_await WhenAny(MoveTemp(LatentExecution), MoveTemp(Cancelling));

	// Wait until the state becomes active (if not already) or invalid
	co_await Latent::Until([this] { return !IsValid(this) || IsStateActive(); });
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

/**
 * Global machine states are used as a supervisor in a finite state machine. <br> <br>
 *
 * Subclass this state explicitely to separate regular machine states from global ones to avoid mistakes.
 */
UCLASS(Abstract)
class FINITESTATEMACHINEMODULE_API UGlobalMachineState
	: public UMachineState
{
	GENERATED_BODY()

protected:
	/** Don't allow users to use these, as they are not meant to be used in global states. */

	//~UMachineState Interface
	virtual void Pushed() override final;
	virtual void Popped() override final;
	virtual void Paused() override final;
	virtual void Resumed() override final;
	//~End of UMachineState Interface
};
