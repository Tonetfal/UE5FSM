# Normal States

## Description

Normal states are meant to execute a specific task. An object most likely is going to have different states: a
door might be locked, closed, opening or open, while a character might be patrolling some area, attacking a player,
or simply dying while shouting its last words.

Users have to override `UMachineState` in order to create their own states. The amount of states is going to vary
depending on your game needs, and most likely not all of them will be contained in a single FSM, but rather spread
out across many FSM's depending on the context.

## Registration

Upon implementing one, it must be registered inside the FSM it's going to be used in using the following method:

```c++
bool UFiniteStateMachine::RegisterState(TSubclassOf<UMachineState> InStateClass);
```

Normal states can be registered at any point of the execution, but it's suggested to do that in the constructor of
the object that contains the FSM.

A better way of doing that is by manually assigning the states to the FSM within BPs. If the FSM is under an AI
controller, you're encouraged to edit the following property:

```c++
TArray<TSubclassOf<UMachineState>> UFiniteStateMachine::InitialStateClassesToRegister;
```

## Initial FSM state

Upon initialization the FSM will enable a state at a specified labels which are defined using the following properties:

```c++
TSubclassOf<UMachineState> UFiniteStateMachine::InitialState = nullptr;
FGameplayTag UFiniteStateMachine::InitialStateLabel = TAG_StateMachine_Label_Default;
```

If it's `nullptr` during initialization, no state will be enabled by default, yet it's possible to enable one manually.

To modify the aforementioned properties use the following method:

```c++
void UFiniteStateMachine::SetInitialState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default);
```

Once again, it's encouraged to edit the properties within BPs.

## Stack

Finite State Machine has a stack of normal states. The top-most state is the one that is running, while all the other 
ones are paused. While a state is paused, no latent code within [labels](Labels.md) is executed. The stack can 
contain any number of states as long as they do **not** repeat, i.e. you cannot push states that are already present 
on the stack.

To interact with the stack there are plenty of methods, they're present in both UFiniteStateMachine and UMachineState:

```c++
bool GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default, bool bForceEvents = true);
bool EndState();
TCoroutine<> PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default, bool* bOutPrematureResult = nullptr);
TCoroutine<> PushStateQueued(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default, FFSM_PushRequestHandle* OutHandle = nullptr);
bool PopState();
```

The methods are relatively simple: they ask the FSM to do something immediately. If the request can be done at that 
specific moment, it'll be executed. However, `PushStateQueued()` is different: push of a valid state might be 
impossible at some point, but even if it is, the request will not fail, instead, it'll wait for an opportunity to do 
that as soon as possible. The client is left with a `FFSM_PushRequestHandle` to manage the pending request if any.

To get more information about these relatively to the labels, read the [labels](Labels.md) documentation, there are 
several sections covering them.

## Events

When the states are being interacted with relatively to the state stack, they will receive different events 
depending on the performed action: begin, end, push, pop, resume, pause.

The signatures:

```c++
void UMachineState::OnBegan(TSubclassOf<UMachineState> OldState);
void UMachineState::OnEnded(TSubclassOf<UMachineState> NewState);
void UMachineState::OnPushed(TSubclassOf<UMachineState> OldState);
void UMachineState::OnPopped(TSubclassOf<UMachineState> NewState);
void UMachineState::OnPaused(TSubclassOf<UMachineState> NewState);
void UMachineState::OnResumed(TSubclassOf<UMachineState> OldState);
```

### OnBegan

The state has been activated using `GotoState()`. Usually the state was not present on the stack beforehand, however 
the event will be fired regardless if the client specifies `bForceEvent` argument inside the `GotoState()` as true.

### OnEnded

The state has been deactivated using `GotoState()` with some other state, OR the FSM has de-initialized. The state was
the top-most state on the stack, but not anymore. Any latent code execution will be terminated.

### OnPushed

The state has been activated using `PushState()`. The state was not present on the stack beforehand, but now it is.

### OnPopped

The state has been deactivated using `PopState()`. The state was the top-most state on the stack, but not anymore. Any
latent code execution will be terminated.

### OnPaused

The state has been paused due to using `PushState()` with some other state. Any latent code execution will be paused. 

### OnResumed

The state has been resumed due to using `PopState()`. The state has become the top-most one on the stack. Any latent 
code execution that has been paused in the Paused state will resume at the point it has stopped at.

## Initialization / De-initialization

When the state either becomes relevant or irrelevant there are different events you can use. 

When a state becomes active, or, in other words, it becomes the top-most state on the stack, the following event is 
called:

```c++
void UMachineState::OnActivated(EStateAction StateAction, TSubclassOf<UMachineState> OldState);
```

When a state becomes inactive, or, in other words, it's not the top-most state on the stack anymore, yet it's present,
the following event is called:

```c++
void UMachineState::OnDectivated(EStateAction StateAction, TSubclassOf<UMachineState> NewState);
```

When a state is added to the stack, i.e. it either began or got pushed on it, the following event is called:

```c++
void UMachineState::OnAddedToStack(EStateAction StateAction, TSubclassOf<UMachineState> OldState);
```

When a state is not present on the stack anymore, i.e. it either ended or got popped out of it, the following event 
is called:

```c++
void UMachineState::OnRemovedFromStack(EStateAction StateAction, TSubclassOf<UMachineState> NewState);
```

Any latent code that is running can be terminated at any point outside that code, and even outside the state itself, 
which increases the machine states boundaries, but makes it harder the code execution management. To do that you can 
use the following method that is present in both `UFiniteStateMachine` and `UMachineState`:

```c++
int32 StopLatentExecution();
```

You can add your custom behavior to it using the following method:
```c++
void UMachineState::StopLatentExecution_Custom(int32 StoppedCoroutines);
```

Anytime a state is removed off the stack, it terminates all labels including their latent code. Note that it is **not**
the case when the state is paused.

## Tips

When creating your own machine state you're encouraged to cache some state specific data. Caching all the objects you 
most likely are going to use in the machine state is going to shorthand a lot of expressions. Example:

```c++
UCLASS()
class UMyMachineState : public UMachineState
{
	GENERATED_BODY()

protected:
	//~UMachineState Interface
	virtual void OnActivated(EStateAction StateAction, TSubclassOf<UMachineState> OldState) override;
	virtual void OnDeactivated(EStateAction StateAction, TSubclassOf<UMachineState> NewState) override;
	//~End of UMachineState Interface
	
protected:
	TWeakObjectPtr<AMyController> Controller = nullptr;
	TWeakObjectPtr<AMyCharacter> Character = nullptr;
	TWeakObjectPtr<UMyGlobalStateData> GlobalStateData = nullptr;
};


void UMyMachineState::OnActivated(EStateAction StateAction, TSubclassOf<UMachineState> OldState)
{
	Super::OnActivated(StateAction, OldState);

	Controller = GetOwnerChecked<AMyController>();
	Character = Controller->GetPawn<AMyCharacter>();
	GlobalStateData = StateMachine->GetStateDataChecked<UMyGlobalStateData, UMyGlobalState>();
	
	check(IsValid(Character));
}

void UMyMachineState::OnDeactivated(EStateAction StateAction, TSubclassOf<UMachineState> NewState)
{
	Controller.Reset();
	Character.Reset();
	GlobalStateData.Reset();
	
	Super::OnDeactivated(StateAction, NewState);
}
```
