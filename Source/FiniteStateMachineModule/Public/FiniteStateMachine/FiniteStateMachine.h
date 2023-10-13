#pragma once

#include "Components/ActorComponent.h"
#include "FiniteStateMachine/MachineState.h"

#include "FiniteStateMachine.generated.h"

using namespace UE5Coro;

DECLARE_LOG_CATEGORY_EXTERN(LogFiniteStateMachine, VeryVerbose, All);

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
UCLASS(ClassGroup=("Finite State Machine"), meta=(BlueprintSpawnableComponent))
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
	virtual void OnRegister() override;
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
	 */
	bool GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = FGameplayTag::EmptyTag,
		bool bForceEvents = true);

	/**
	 * Go to a requested label using the active state.
	 * @param	Label label to go to.
	 * @return	If true, label has been activated, false otherwise.
	 */
	bool GotoLabel(FGameplayTag Label);

	/**
	 * Push a specified state at a requested label on top of the stack.
	 * @param	InStateClass state to push.
	 * @param	Label label to start the state at.
	 */
	TCoroutine<bool> PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = FGameplayTag::EmptyTag);

	/**
	 * Pop the top-most state from stack.
	 * @return	If true, a state has been popped, false otherwise.
	 */
	bool PopState();

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
	 * Check whether a given state is registered in this state machine.
	 * @param	InStateClass state to check against.
	 * @return	If true, the state is reigstered, false otherwise.
	 */
	bool IsStateRegistered(TSubclassOf<UMachineState> InStateClass) const;

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
	 * @param	InStateClass state which data has to be retrieved.
	 * @return	State data. May be nullptr.
	 */
	template<typename StateDataClass>
	StateDataClass* GetStateData(TSubclassOf<UMachineState> InStateClass) const;

	/**
	 * Get typed data of a given state checked.
	 * @tparam	StateDataClass state data type to search for.
	 * @param	InStateClass state which data has to be retrieved.
	 * @return	State data checked.
	 */
	template<typename StateDataClass>
	StateDataClass* GetStateDataChecked(TSubclassOf<UMachineState> InStateClass) const;

	/**
	 * Get physical actor of the state machine.
	 * @return	Physical actor. If failed to find the avatar, owner will be returned instead.
	 * @note	Supports only Controller and PlayerState.
	 */
	AActor* GetAvatar() const;

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
	TCoroutine<bool> PushState_Implementation(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label);

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
	TCoroutine<bool> WaitUntilStateAction(TSubclassOf<UMachineState> InStateClass, EStateAction StateAction) const;

public:
	/** All the machine states that will be automatically registered on initialization. */
	UPROPERTY(EditDefaultsOnly, Category="State Machine")
	TArray<TSubclassOf<UMachineState>> InitialStateClassesToRegister;

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

	/** If true, initial states have been activated, false otherwise. */
	bool bActiveStatesBegan = false;

	/** If true, important internal state is being modified, false otherwise. */
	bool bModifyingInternalState = false;
};

template<typename T>
T* UFiniteStateMachine::GetStateData(TSubclassOf<UMachineState> InStateClass) const
{
	static_assert(TIsDerivedFrom<T, UMachineStateData>::IsDerived,
		"Attempting to pass a templated parameter that is not of UMachineStateData class.");

	UMachineStateData* StateData = GetStateData(InStateClass, T::StaticClass());
	auto* TypedStateData = Cast<T>(StateData);
	return TypedStateData;
}

template<typename T>
T* UFiniteStateMachine::GetStateDataChecked(TSubclassOf<UMachineState> InStateClass) const
{
	static_assert(TIsDerivedFrom<T, UMachineStateData>::IsDerived,
		"Attempting to pass a templated parameter that is not of UMachineStateData class.");

	auto* TypedStateData = GetStateData<T>(InStateClass);
	check(IsValid(TypedStateData));

	return TypedStateData;
}
