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
TCoroutine<> PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default, bool* bOutPrematureResult = nullptr);
bool PopState();
```

To get more information about these relatively to the labels, read the [labels](Labels.md) documentation, there are 
several sections covering them.

## Events

When the states are being interacted with relatively to the state stack, they will receive different events 
depending on the performed action: begin, end, push, pop, resume, pause.

The signatures:

```c++
void UMachineState::Begin(TSubclassOf<UMachineState> PreviousState);
void UMachineState::End(TSubclassOf<UMachineState> NewState);
void UMachineState::Pushed();
void UMachineState::Popped();
void UMachineState::Paused();
void UMachineState::Resumed();
```

### Begin

The state has been activated using `GotoState()`. Usually the state was not present on the stack beforehand, however 
the event will be fired regardless if the client specifies `bForceEvent` argument inside the `GotoState()` as true.

### End

The state has been deactivated using `GotoState()` with some other state, OR the FSM has de-initialized. The state was
the top-most state on the stack, but not anymore. Any latent code execution will be terminated.

### Pushed

The state has been activated using `PushState()`. The state was not present on the stack beforehand, but now it is.

### Popped

The state has been deactivated using `PopState()`. The state was the top-most state on the stack, but not anymore. Any
latent code execution will be terminated.

### Paused

The state has been paused due to using `PushState()` with some other state. Any latent code execution will be paused. 

### Resumed

The state has been resumed due to using `PopState()`. The state has become the top-most one on the stack. Any latent 
code execution that has been paused in the Paused state will resume at the point it has stopped at.

## Initialization / De-initialization

When the state either becomes relevant or irrelevant there are different events you can use. 

When the state gets the `Begin` or `Pushed` events the following event is called:

```c++
void UMachineState::InitState();
```

When the state gets the `End` or `Popped` events the following event is called:

```c++
void UMachineState::ClearState();
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
	virtual void InitState() override;
	virtual void ClearState() override;
	//~End of UMachineState Interface
	
protected:
	TWeakObjectPtr<AMyController> Controller = nullptr;
	TWeakObjectPtr<AMyCharacter> Character = nullptr;
	TWeakObjectPtr<UMyGlobalStateData> GlobalStateData = nullptr;
};


void UMyMachineState::InitState()
{
	Super::InitState();

	Controller = GetOwnerChecked<AMyController>();
	Character = Controller->GetPawn<AMyCharacter>();
	GlobalStateData = StateMachine->GetStateDataChecked<UMyGlobalStateData, UMyGlobalState>();
	
	check(IsValid(Character));
}

void UMyMachineState::ClearState()
{
	Controller.Reset();
	Character.Reset();
	GlobalStateData.Reset();
	
	Super::ClearState();
}
```
