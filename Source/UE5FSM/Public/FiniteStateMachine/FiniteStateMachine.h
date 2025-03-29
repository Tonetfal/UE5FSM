#pragma once

#include "Components/ActorComponent.h"
#include "FiniteStateMachine/MachineState.h"

#include "FiniteStateMachine.generated.h"

UE5FSM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateMachine_Label_Default);

UENUM()
enum class EFSM_PendingPushRequestResult : uint8
{
	Success,
	Canceled
};

UE5FSM_API DECLARE_MULTICAST_DELEGATE_OneParam(FOnPendingPushRequestSignature, EFSM_PendingPushRequestResult Result);

/**
 * Finite state machine's push request handle used to listen for the push result and to cancel the request.
 */
struct UE5FSM_API FFSM_PushRequestHandle
{
public:
	friend UFiniteStateMachine;

public:
	/**
	 * Bind a custom callback to the OnResult event.
	 * @param	Callback delegate to call when pending request is executed.
	 */
	void BindOnResultCallback(const FOnPendingPushRequestSignature::FDelegate&& Callback) const;

	/**
	 * Cancel the request if it's pending.
	 */
	void Cancel() const;

	/**
	 * Check whether this request is pending.
	 * @return	If true, it's pending, false otherwise.
	 */
	bool IsPending() const;

private:
	inline static uint32 s_ID = 0;
	uint32 ID = 0;
	TWeakObjectPtr<UFiniteStateMachine> StateMachine = nullptr;
};

/**
 * Component to manage Machine States defining behavior of a stateful object in an easy way.
 *
 * # The Finite State Machine can be only in one single normal state a time, making the management easier. To learn more
 * about states, read the UMachineState documentation.
 *
 * # There are normal and global states:
 * - Normal state: just a regular state that can be activated/deactivated, paused/resumed, pushed/popped on play time.
 * - Global state: a state that is active throughout the whole lifetime of the state machine, that can act as a
 * supervisor on normal states and the object itself.
 *
 * # States management:
 * - States must be registered manually.
 *   - It's encouraged to define them in blueprints to avoid hard coding.
 *   - If you want to add them in C++ though, use the RegisterState() function before the initialization terminates.
 *   You should do it in the constructor of the owning object, as it allows the reflection system to see the states your
 *   state machine contains.
 * - Initial states (both global and normal) should be assigned manually as well.
 *   - It's encouraged to define them in blueprints to avoid hard coding.
 *   - If you want to assign them in C++ though, use the SetInitialState() and SetGlobalState(). Note that global state
 *   can be set during initialization only, while normal states can be switched at any time after initialization.
 * - To switch behaviors use GotoState(), PushState(), PopState(), PauseState(), ResumeState(), and GotoLabel().
 * - To access data state use GetStateData().
 */
UCLASS(Config="Engine", DefaultConfig, ClassGroup=("Finite State Machine"), meta=(BlueprintSpawnableComponent))
class UE5FSM_API UFiniteStateMachine
	: public UActorComponent
{
	GENERATED_BODY()

public:
	enum class EPushRequestResult : uint8
	{
		Success,
		Canceled
	};

private:
	DECLARE_MULTICAST_DELEGATE_TwoParams(
		FOnPushRequestResultSignature,
		int32 RequestID,
		EPushRequestResult Result);

private:
	struct FPendingPushRequest
	{
	public:
		FPendingPushRequest() = default;
		explicit FPendingPushRequest(uint32 InID)
			: ID(InID) { }

		bool operator==(const FPendingPushRequest& Rhs) const
		{
			return ID == Rhs.ID;
		}

	public:
		uint32 ID = 0;
		TSubclassOf<UMachineState> StateClass = nullptr;
		FGameplayTag Label = FGameplayTag::EmptyTag;
	};

public:

#ifdef WITH_EDITOR
	/** Structure to hold some debug information. */
	struct FDebugStateAction
	{
	public:
		TWeakObjectPtr<UMachineState> State = nullptr;
		EStateAction Action = EStateAction::None;
		float ActionTime = -1.f;
	};
#endif

public:
	UFiniteStateMachine();

	//~UActorComponent Interface
	virtual void PostReinitProperties() override;
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
	void SetInitialState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default);

	/**
	 * Set global state the state machine will be in.
	 * @param	InStateClass global state to be in throughout whole execution.
	 * @note	Only callable before the initialiation terminates.
	 */
	void SetGlobalState(TSubclassOf<UMachineState> InStateClass);

	/**
     * Activate a state at a specified label. If there's any active state, it'll deactivated.
	 * @param	InStateClass state to go to.
	 * @param	Label label to start the state at.
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
	 * @note	The requested label will be activated the next tick, if not changed with a subsequent GotoLabel call.
	 */
	bool GotoLabel(FGameplayTag Label);

	/**
	 * Push a state at a specified label on top of the stack. If there's any active state, it'll be paused upon
	 * successful push.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state at.
	 * @param	bOutPrematureResult output parameter. Boolean to use when you want to use the function result before it
	 * returns code execution.
	 */
	UE5Coro::TCoroutine<> PushState(TSubclassOf<UMachineState> InStateClass,
		FGameplayTag Label = TAG_StateMachine_Label_Default, bool* bOutPrematureResult = nullptr);

	/**
	 * Push a state at a specified label on top of the stack. If the operation is not possible to execute for
	 * any reason that might change in the future, it'll queued, and apply it as soon as it becomes possible following
	 * the existing queue order. If there's any active state, it'll be paused upon successful push.
	 * @param	OutHandle output parameter. Push request handle used to interact with the request.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state at.
	 */
	UE5Coro::TCoroutine<> PushStateQueued(FFSM_PushRequestHandle& OutHandle, TSubclassOf<UMachineState> InStateClass,
		FGameplayTag Label = TAG_StateMachine_Label_Default);

	/**
	 * Pop the top-most state from stack. If there's any state below this in the stack, it'll resume its execution.
	 * @return	If true, a state has been popped, false otherwise.
	 * @note	Unlike Unreal 3, when succeeds, it doesn't interrupt latent code execution the function is called
	 * from (if any). It is the caller obligation to call co_return (or simply not have any code after a successful
	 * PopState).
	 */
	bool PopState();

	/**
	 * Clear all states from the stack leaving it empty.
	 * @return	Amount of ended states.
	 */
	int32 ClearStack();

	/**
	 * Stop any latent execution of EVERY state known to this state machine. Doesn't interrupt label execution.
	 * @return	Amount of latent executions stopped.
	 *
	 * @note	If a latent execution is terminated while the state is paused, it's not going to carry on the execution
	 * unless the state becomes active.
	 * @see		UMachineState::RunLatentExecution()
	 */
	int32 StopEveryLatentExecution();

	/**
	 * Stop all latent functions (coroutines) of EVERY state that is present on stack regardless of the state.
	 * @return	Amount of latent executions stopped.
	 */
	int32 StopEveryRunningLabel();

	/**
	 * Remove the specified request from the pending push list.
	 * @param	Handle request to cancel.
	 * @return	If true, the request has been successfully canceled, false otherwise.
	 */
	bool CancelPushRequest(FFSM_PushRequestHandle Handle);

	/**
	 * Check whether the push request is pending.
	 * @param	Handle request to check.
	 * @return	If true, the request is pending, false otherwise.
	 */
	bool IsPushRequestPending(FFSM_PushRequestHandle Handle) const;

	/**
	 * Get multicast delegate that is fired when the specified push request handle is finished. If the handle is not
	 * associated with an active pending request, the returned delegate will never fire.
	 * @param	Handle request the delegate will be returned for.
	 * @return	OnPendingPushRequesteResult delegate.
	 */
	FOnPendingPushRequestSignature& GetOnPendingPushRequestResultDelegate(FFSM_PushRequestHandle Handle);

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
	 * Get state the state machine starts with by default.
	 * @return	Initial machine state.
	 */
	TSubclassOf<UMachineState> GetInitialMachineState() const;

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
	 * @tparam	StateDataClass state data type to search for.
	 * @tparam	StateClass state which data has to be retrieved.
	 * @return	State data. May be nullptr.
	 */
	template<typename StateDataClass, typename StateClass>
	StateDataClass* GetStateData() const;

	/**
	 * Get typed data of a given state checked.
	 * @tparam	StateDataClass state data type to search for.
	 * @tparam	StateClass state which data has to be retrieved.
	 * @return	State data checked.
	 */
	template<typename StateDataClass, typename StateClass>
	StateDataClass* GetStateDataChecked() const;

	/**
	 * Get states stack.
	 * @return	States stack.
	 */
	const TArray<TSubclassOf<UMachineState>>& GetStatesStack() const;

	/**
	 * Get registered state classes.
	 * @return	Registered state classes.
	 */
	TArray<TSubclassOf<UMachineState>> GetRegisteredStateClasses() const;

	/**
	 * Get global state class.
	 * @return	Global state class.
	 */
	TSubclassOf<UMachineState> GetGlobalStateClass() const;

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

	/**
	 * Shorthand for GetWorld()->GetTimerManager().
	 */
	FTimerManager& GetTimerManager() const;

#ifdef WITH_EDITOR
	/**
	 * Get last actions
	 */
	TArray<FDebugStateAction> GetLastStateActionsStack() const;
#endif

protected:
	/**
	 * Called when our state performs an action.
	 * @param	State state has that performed an action.
	 * @param	StateAction the performed action.
	 */
	virtual void OnStateAction(UMachineState* State, EStateAction StateAction);

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
	UE5Coro::TCoroutine<> GotoState_LatentImplementation(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label,
		bool bForceEvents);

	/**
	 * Push a specified state at a requested label on top of the stack. Doesn't perform any check.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state at.
	 */
	UE5Coro::TCoroutine<> PushState_Implementation(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label);
	UE5Coro::TCoroutine<> PushState_LatentImplementation(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label);

	/**
	 * Pop the top-most state from stack. Doesn't perform any check.
	 * @return	If true, a state has been popped, false otherwise.
	 */
	void PopState_Implementation();
	UE5Coro::TCoroutine<> PopState_LatentImplementation();

	/**
	 * End the active state. Doesn't perform any check.
	 * @return	If true, a state has ended, false otherwise.
	 */
	void EndState_Implementation();
	UE5Coro::TCoroutine<> EndState_LatentImplementation();

	/**
	 * Wait until a specified state gets a specified state action.
	 * @param	InStateClass state which state action to wait.
	 * @param	StateAction state action to wait.
	 */
	UE5Coro::TCoroutine<> WaitUntilStateAction(TSubclassOf<UMachineState> InStateClass, EStateAction StateAction) const;

	/**
	 * Wait until the active state dispatches an event that it is *currently* dispatching.
	 */
	UE5Coro::TCoroutine<> WaitUntilActiveStateEventDispatch();

	/**
	 * Push a request in the queue
	 * @param	OutHandle output parameter. Push request handle used to interact with the request.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state at.
	 */
	UE5Coro::TCoroutine<> AddAndWaitPendingPushRequest(FFSM_PushRequestHandle& OutHandle,
		TSubclassOf<UMachineState> InStateClass, FGameplayTag Label);

	/**
	 * Try to push the first element in the queue.
	 */
	void UpdatePushQueue();

	/**
	 * Try to execute a pending push request.
	 * @param	Request request to try to execute.
	 */
	void PushState_Pending(FPendingPushRequest Request);

	/**
	 * Remove all latent execution cancel delegates that are no longer bound because the latent execution the delegate
	 * was associated with has terminated before the user has used it.
	 * @return	Amount of removed invalid latent execution cancellers.
	 */
	void ClearStatesInvalidLatentExecutionCancellers();

	/**
	 * Check whether active state allows to transit to given state.
	 * @param	InStateClass state to check the transition for.
	 * @return	If true, transition is allowed, false otherwise.
	 */
	bool IsTransitionBlockedTo(TSubclassOf<UMachineState> InStateClass) const;

	/**
	 * Check whether active state can be safely deactived.
	 * @param	OutReason output parameter. The reason it cannot deactivate if so.
	 * @return	If true, it can safely deactivate, false otherwise.
	 */
	bool CanActiveStateSafelyDeactivate(FString& OutReason) const;

	/**
	 * Check whether the active state is dispatching an event (begin, end, push, pop, pause, resume).
	 * @return	If true, it's not safe to deactivate it, false otherwise.
	 */
	bool IsActiveStateDispatchingEvent() const;

private:
	/** Called when a pending push request is completed with any result. */
	FOnPushRequestResultSignature OnPushRequestResultDelegate;

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
	TWeakObjectPtr<UMachineState> ActiveGlobalState = nullptr;

	/** Active normal state. */
	UPROPERTY(VisibleInstanceOnly, Category="State Machine|Debug")
	TWeakObjectPtr<UMachineState> ActiveState = nullptr;

	/** Machine states stack. The top-most is the active one, while all the others are paused. */
	TArray<TSubclassOf<UMachineState>> StatesStack;

private:
	/** Global state the state machine is in. If not specified, it won't have any. */
	UPROPERTY(EditDefaultsOnly, Category="State Machine",
		meta=(MustImplement="/Script/UE5FSM.GlobalMachineStateInterface"))
	TSubclassOf<UMachineState> GlobalStateClass = nullptr;

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

	/** Time in seconds it takes to start clearing state execution cancellers. */
	UPROPERTY(Config)
	float StateExecutionCancellersClearingInterval = 60.f;

#ifdef WITH_EDITOR
	/** Container of all states that performed an action. It exists for debug purposes only. */
	std::queue<FDebugStateAction> LastStateActionsStack;
#endif

	/** Queue for the pending push requests that failed to happen. Only the very first entry is tried to be pushed. */
	TArray<FPendingPushRequest> PendingPushRequests;

	/** Delegates to pending push request ID. Users can listen for these using ID to register their callbacks. */
	TMap<uint32, FOnPendingPushRequestSignature> OnPendingPushRequestResultDelegates;

	/** Timer associated with clearing state execution cancellers process. */
	FTimerHandle CancellersCleaningTimerHandle;

	/**
	 * If true, a latent request (GotoState, EndState, PushState, or PopState) is running, and it's not safe to run
	 * another one, false otherwise.
	 */
	bool bIsRunningLatentRequest = false;
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

template<typename StateDataClass, typename StateClass>
StateDataClass* UFiniteStateMachine::GetStateData() const
{
	static_assert(TIsDerivedFrom<StateDataClass, UMachineStateData>::IsDerived,
		"Attempting to pass a templated parameter that is not of UMachineStateData class.");
	static_assert(TIsDerivedFrom<StateClass, UMachineState>::IsDerived,
		"Attempting to pass a templated parameter that is not of UMachineState class.");

	UMachineStateData* StateData = GetStateData(StateClass::StaticClass(), StateDataClass::StaticClass());
	auto* TypedStateData = Cast<StateDataClass>(StateData);
	return TypedStateData;
}

template<typename StateDataClass, typename StateClass>
StateDataClass* UFiniteStateMachine::GetStateDataChecked() const
{
	auto* TypedStateData = GetStateData<StateDataClass, StateClass>();
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
