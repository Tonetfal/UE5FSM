# States

States are the crucial part of the Finite State Machine, they're like bricks you build your functionality upon. A 
state machine can have any number of states which are used for a variety of tasks. Each one of them is meant to be 
specialized in a certain task - Patrolling, Attacking, Dying, and many others. You can freely switch between them 
back and forth depending on your specific needs.

## Types

There are two different types of states: [normal](NormalState.md) and [global](GlobalState.md). Regardless of the state 
type, each one of them has something in common: [State Data](StateData.md) and [Labels](Labels.md).

## Manipulating the states stack

The states stack of an FSM can be manipulated using either the component itself, or the internally from the machine
states. The signatures of the methods are the same.

### GotoState

```c++
bool GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default, bool bForceEvents = true);
```

Activate a state at a specified label. If there's any active state, it'll deactivated.

- **InStateClass** - State to go to.
- **Label** - Label to start the state at.
- **bForceEvents** - In case of switching to the same state we're in: If true, fire end & begin events, otherwise do
  not.
- **Return Value** - If true, label has been activated, false otherwise.

### EndState

```c++
bool EndState();
```

End the active state. If there's any state below this in the stack, it'll resume its execution.

- **Return Value** - If true, a state has ended, false otherwise.

### PushState

```c++
UE5Coro::TCoroutine<> PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default, bool* bOutPrematureResult = nullptr);
```

Push a state at a specified label on top of the stack. If there's any active state, it'll be paused upon successful 
push.

- **InStateClass** - State to push.
- **Label** - Label to start the state with.
- **bOutPrematureResult** - Output parameter. Boolean to use when you want to use the function result before it returns
  code execution.

### PushStateQueued

```c++
UE5Coro::TCoroutine<> PushStateQueued(FFSM_PushRequestHandle& OutHandle, TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default);
```

Push a state at a specified label on top of the stack. If the operation is not possible to execute for
any reason that might change in the future, it'll queued, and apply it as soon as it becomes possible following
the existing queue order. If there's any active state, it'll be paused upon successful push.

- **OutHandle** - Output parameter. Push request handle used to interact with the request.
- **InStateClass** - State to push.
- **Label** - Label to start the state at.

### PopState

```c++
bool PopState();
```

Pop the top-most state from stack. If there's any state below this in the stack, it'll resume its execution.

- **Return Value** - If true, a state has been popped, false otherwise.

## Limitations

States stack manipulations is not always possible. The line between the safe and unsafe states lies within a machine 
state event, such as Begin, End, Push, Pop, Resume, or Pause. In fact, it's allowed to invoke any aforementioned 
function, but only once, up until it doesn't execute. Examples:

```c++
void UMyMachineState1::OnBegan(TSubclassOf<UMachineState> OldState)
{
    Super::OnBegan(OldState);
    
    // It's fine to call one GotoState(), note that it'll start executing once the event is finished
    GotoState(UMyMachineState2::StaticClass());
    
    // These will be ignored! We just requested to manipalute the stack.
    GotoState(UMyMachineState2::StaticClass());
    PushState(UMyMachineState3::StaticClass());
    EndState();
    PopState();
    
    // Note that you're still allowed to use the queued push state as it's a unique action
	FFSM_PushRequestHandle Handle;
    PushStateQueued(Handle, UMyMachineState3::StaticClass());
}

void UMyMachineState1::OnEnded(TSubclassOf<UMachineState> NewState)
{
    Super::OnEnded(NewState);
    
    // This will be ignored! We have already requested a Goto in UMyMachineState1::OnBegan().
    GotoState(UMyMachineState2::StaticClass());
}

void UMyMachineState2::OnBegan(TSubclassOf<UMachineState> OldState)
{
    Super::OnBegan(OldState);
    
    // This will be ignored! We have already requested a Goto in UMyMachineState1::OnBegan(), and it's about to finish,
    // as it was requested to Goto this state.
    GotoState(UMyMachineState3::StaticClass());
}
```
