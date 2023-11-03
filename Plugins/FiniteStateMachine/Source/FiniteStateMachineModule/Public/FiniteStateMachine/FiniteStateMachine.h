#pragma once

#include "Components/ActorComponent.h"
#include "FiniteStateMachine/MachineState.h"

#include "FiniteStateMachine.generated.h"

using namespace UE5Coro;

/**
 * Component to manage Machine States defining behavior of an object in an easy way.
 *
 * # The Finite State Machine can be only in one single normal state a time, making the management easier. To learn more
 * about states, read the UMachineState documentation.
 *
 * # There are normal and global states:
 * - Normal state: just a regular state that can be activated/deactivated, paused/resumed, pushed/popped on play time.
 * - Global state: a state that is active throught the whole lifetime of the state machine, that can act as a supervisor
 * on normal states.
 *
 * # States management:
 * - States must be registered manually.
 *   - It's encouraged to define them in blueprints to avoid annoying hard coding.
 *   - If you want to add them in C++ though, use the RegisterState() function before the initialization terminates.
 *   You should do it in the constructor of the owning object, as it allows the reflection system to see the states your
 *   state machine contains.
 * - Initial states (both global and normal) should be assigned manually as well
 *   - It's encouraged to define them in blueprints to avoid annoying hard coding.
 *   - If you want to assign them in C++ though, use the SetInitialState() and SetGlobalState(). Note that global state
 *   can be set during initialization only, while normal states can be switched at any time after initialization.
 * - To switch behaviors use GotoState(), PushState(), PopState(), PauseState(), ResumeState(), and GotoLabel().
 * - To access data a state wants to expose, use the GetStateData().
 */
UCLASS(Config="Engine", DefaultConfig, ClassGroup=("Finite State Machine"), meta=(BlueprintSpawnableComponent))
class FINITESTATEMACHINEMODULE_API UFiniteStateMachine
	: public UActorComponent
{
	GENERATED_BODY()

private:
	/** Disallows to enter methods that change internal state while already changing it. */
	friend struct FFiniteStateMachineMutex;

public:
	UFiniteStateMachine();

	//~UActorComponent Interface
	virtual void Activate(bool bReset) override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick LevelTick,
		FActorComponentTickFunction* ActorComponentTickFunction) override;
	//~End of UActorComponent Interface

	/**
	 * Register a given state.
	 * @param	InStateClass state to register.
	 * @return	If true, state has been successfully registered, false otherwise.
	 */
	bool RegisterState(TSubclassOf<UMachineState> InStateClass);

	/**
	 * Set initial state the state machine starts with.
	 * @param	InStateClass state to start with.
	 * @param	Label label to start the state at.
	 */
	void SetInitialState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = FGameplayTag::EmptyTag);

	/**
	 * Set global state the state machine will be in.
	 * @param	InStateClass global state to be in throughout whole execution.
	 * @note	Only callable before the initialiation terminates.
	 */
	void SetGlobalState(TSubclassOf<UGlobalMachineState> InStateClass);

	/**
	 * Go to a requested state at a requested label.
	 * @param	InStateClass state to go to.
	 * @param	Label label to start the state at.
	 * @param	bForceEvents in case of switching to the same state we're in: If true, fire end & begin events,
	 * otherwise do not.
	 * @return	If true, state has been successfully switched, false otherwise.
	 * @note	Unlike Unreal 3, when succeeds, it doesn't interrupt latent code execution the function is called
	 * from (if any). It is the caller obligation to call co_return (or simply not have any code after a successful
	 * GotoState).
	 */
	bool GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = FGameplayTag::EmptyTag,
		bool bForceEvents = true);

	/**
	 * Go to a requested label using the active state.
	 * @param	Label label to go to.
	 * @return	If true, label has been activated, false otherwise.
	 * @note	The requested label will be activated the next tick, if not changed with a subsequent GotoLabel call.
	 */
	bool GotoLabel(FGameplayTag Label);

	/**
	 * Push a specified state at a requested label on top of the stack.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state at.
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
	 * Stop any latent execution of EVERY state known to this state machine. Doesn't interrupt label execution.
	 * @return	Amount of latent executions stopped.
	 *
	 * @note	If a latent execution is terminated while the state is paused, it's not going to carry on the execution
	 * unlles the state becomes active.
	 * @see		UMachineState::LatentExecution()
	 */
	int32 StopEveryLatentExecution();

	/**
	 * Stop all latent functions (coroutines) of EVERY state that is present on stack regardless of the state.
	 * @return	Amount of latent executions stopped.
	 */
	int32 StopEveryRunningLabels();

	/**
	 * Check whether a given state is active.
	 * @param	InStateClass state to check against.
	 * @param	bCheckStack if true, whole state stack will be used, otherwise only the active state will be checked.
	 * @return	If true, the state is currently active, false otherwise.
	 * @note	When checking whole state stack, "active" is treated as "present", i.e. a state can be either executing
	 * or paused.
	 */
	bool IsInState(TSubclassOf<UMachineState> InStateClass, bool bCheckStack = false) const;

	/**
	 * Get class of the active normal state.
	 * @return	Class of the active normal state.
	 */
	TSubclassOf<UMachineState> GetActiveStateClass() const;

	/**
	 * Check whether a given state class is registered in this state machine.
	 * @param	InStateClass state to check against.
	 * @return	If true, the state is reigstered, false otherwise.
	 */
	bool IsStateRegistered(TSubclassOf<UMachineState> InStateClass) const;

	/**
	 * Get a registered state of a given class.
	 * @param	InStateClass state class to search retrieve.
	 * @return	State of the given class. Might be nullptr.
	 */
	UMachineState* GetState(TSubclassOf<UMachineState> InStateClass) const;

	/**
	 * Get a registered state of a given class.
	 * @tparam	UserClass state class to search retrieve.
	 * @return	State of the given class. Might be nullptr.
	 */
	template<typename UserClass>
	UserClass* GetState() const;

	/**
	 * Get a registered state of a given class checked.
	 * @tparam	UserClass state class to search retrieve.
	 * @return	State of the given class checked.
	 */
	template<typename UserClass>
	UserClass* GetStateChecked() const;

	/**
	 * Get data of a given state of a specified type.
	 * @param	InStateClass state which data has to be retrieved.
	 * @param	InStateDataClass state data type to search for.
	 * @return	State data. May be nullptr.
	 */
	UMachineStateData* GetStateData(TSubclassOf<UMachineState> InStateClass,
		TSubclassOf<UMachineStateData> InStateDataClass) const;

	/**
	 * Get typed data of a given state.
	 * @tparam	UserClass state data type to search for.
	 * @param	InStateClass state which data has to be retrieved.
	 * @return	State data. May be nullptr.
	 */
	template<typename UserClass>
	UserClass* GetStateData(TSubclassOf<UMachineState> InStateClass) const;

	/**
	 * Get typed data of a given state checked.
	 * @tparam	UserClass state data type to search for.
	 * @param	InStateClass state which data has to be retrieved.
	 * @return	State data checked.
	 */
	template<typename UserClass>
	UserClass* GetStateDataChecked(TSubclassOf<UMachineState> InStateClass) const;

	/**
	 * Get physical actor of the state machine.
	 * @return	Physical actor. If failed to find the avatar, owner will be returned instead.
	 * @note	Supports only Controller and PlayerState.
	 */
	AActor* GetAvatar() const;

	/**
	 * Get typed physical actor of the state machine.
	 * @return	Typed physical actor. If failed to find the avatar, owner will be returned instead.
	 * @note	Supports only Controller and PlayerState.
	 */
	template<typename T>
	T* GetAvatar() const;

	/**
	 * Get typed physical actor of the state machine checked.
	 * @return	Typed physical actor checked. If failed to find the avatar, owner will be returned instead.
	 * @note	Supports only Controller and PlayerState.
	 */
	template<typename T>
	T* GetAvatarChecked() const;

private:
	/**
	 * Activate initial states.
	 */
	void BeginActiveStates();

	/**
	 * Register a given state. Doesn't perform any check.
	 * @param	InStateClass state to register.
	 * @return	If true, state has been successfully registered, false otherwise.
	 */
	UMachineState* RegisterState_Implementation(TSubclassOf<UMachineState> InStateClass);

	/**
	 * Find a given state.
	 * @param	InStateClass state to search for.
	 * @return	Found state. May be nullptr.
	 */
	UMachineState* FindState(TSubclassOf<UMachineState> InStateClass) const;

	/**
	 * Find a given state checked.
	 * @param	InStateClass state to search for.
	 * @return	Found state checked.
	 */
	UMachineState* FindStateChecked(TSubclassOf<UMachineState> InStateClass) const;

	/**
	 * Go to a requested state at a requested label. Doesn't perform any check.
	 * @param	InStateClass state to go to.
	 * @param	Label label to start the state at.
	 * @param	bForceEvents in case of switching to the same state we're in: If true, fire end & begin events,
	 * otherwise do not.
	 */
	void GotoState_Implementation(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label, bool bForceEvents);

	/**
	 * Push a specified state at a requested label on top of the stack. Doesn't perform any check.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state at.
	 */
	TCoroutine<> PushState_Implementation(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label);

	/**
	 * Pop the top-most state from stack. Doesn't perform any check.
	 * @return	If true, a state has been popped, false otherwise.
	 */
	void PopState_Implementation();

	/**
	 * Wait until a specified state gets a specified state action.
	 * @param	InStateClass state which state action to wait.
	 * @param	StateAction state action to wait.
	 */
	TCoroutine<> WaitUntilStateAction(TSubclassOf<UMachineState> InStateClass, EStateAction StateAction) const;

	/**
	 * Remove all latent execution cancel delegates that are no longer bound because the latent execution the delegate
	 * was associated with has terminated before the user has used it.
	 * @return	Amount of removed invalid latent execution cancellers.
	 */
	void ClearStatesInvalidLatentExecutionCancellers();

public:
	/** All the machine states that will be automatically registered on initialization. */
	UPROPERTY(EditDefaultsOnly, Category="State Machine")
	TArray<TSubclassOf<UMachineState>> InitialStateClassesToRegister;

protected:
	/** All the registered states. */
	UPROPERTY(VisibleInstanceOnly, Category="State Machine|Debug")
	TArray<TObjectPtr<UMachineState>> RegisteredStates;

	/** Active global state. */
	UPROPERTY(VisibleInstanceOnly, Category="State Machine|Debug")
	TWeakObjectPtr<UGlobalMachineState> ActiveGlobalState = nullptr;

	/** Active normal state. */
	UPROPERTY(VisibleInstanceOnly, Category="State Machine|Debug")
	TWeakObjectPtr<UMachineState> ActiveState = nullptr;

	/** Machine states stack. The top-most is the active one, while all the others are paused. */
	TArray<TSubclassOf<UMachineState>> StatesStack;

private:
	/** Global state the state machine is in. If not specified, it won't have any. */
	UPROPERTY(EditDefaultsOnly, Category="State Machine")
	TSubclassOf<UGlobalMachineState> GlobalStateClass = nullptr;

	/**
	 * Initial state the state machine starts with. If not specified, it won't have any unless GotoState() is used after
	 * initialization.
	 */
	UPROPERTY(EditDefaultsOnly, Category="State Machine")
	TSubclassOf<UMachineState> InitialState = nullptr;

	/**
	 * Label the initial state starts with. If not specified, Default label will be used.
	 */
	UPROPERTY(EditDefaultsOnly, Category="State Machine", meta=(Categories="StateMachine.Label"))
	FGameplayTag InitialStateLabel = TAG_StateMachine_Label_Default;

	/** If true, initial states have been activated, false otherwise. */
	bool bActiveStatesBegan = false;

	/** If true, important internal state is being modified, false otherwise. */
	bool bModifyingInternalState = false;

	/** Time in seconds it takes to start clearing state execution cancellers. */
	UPROPERTY(Config)
	float StateExecutionCancellersClearingInterval = 60.f;

	/** Timer associated with clearing state execution cancellers process. */
	FTimerHandle CancellersCleaningTimerHandle;
};

template<typename UserClass>
UserClass* UFiniteStateMachine::GetState() const
{
	static_assert(TIsDerivedFrom<UserClass, UMachineState>::IsDerived,
		"Attempting to pass a templated parameter that is not of UMachineState class.");

	auto* State = GetState(UserClass::StaticClass());
	auto* TypedState = CastChecked<UserClass>(State);
	return TypedState;
}

template<typename UserClass>
UserClass* UFiniteStateMachine::GetStateChecked() const
{
	auto* TypedState = GetState<UserClass>();
	check(IsValid(TypedState));

	return TypedState;
}

template<typename UserClass>
UserClass* UFiniteStateMachine::GetStateData(TSubclassOf<UMachineState> InStateClass) const
{
	static_assert(TIsDerivedFrom<UserClass, UMachineStateData>::IsDerived,
		"Attempting to pass a templated parameter that is not of UMachineStateData class.");

	UMachineStateData* StateData = GetStateData(InStateClass, UserClass::StaticClass());
	auto* TypedStateData = Cast<UserClass>(StateData);
	return TypedStateData;
}

template<typename UserClass>
UserClass* UFiniteStateMachine::GetStateDataChecked(TSubclassOf<UMachineState> InStateClass) const
{
	auto* TypedStateData = GetStateData<UserClass>(InStateClass);
	check(IsValid(TypedStateData));

	return TypedStateData;
}

template<typename UserClass>
UserClass* UFiniteStateMachine::GetAvatar() const
{
	static_assert(TIsDerivedFrom<UserClass, AActor>::IsDerived,
		"Attempting to pass a templated parameter that is not of AActor class.");

	AActor* Avatar = GetAvatar();
	auto* TypedAvatar = Cast<UserClass>(Avatar);
	return TypedAvatar;
}

template<typename UserClass>
UserClass* UFiniteStateMachine::GetAvatarChecked() const
{
	auto* TypedAvatar = GetAvatar<UserClass>();
	check(IsValid(TypedAvatar));

	return TypedAvatar;
}
