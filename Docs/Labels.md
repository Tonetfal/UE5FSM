# Labels

Labels are special functions the
[coroutines](https://github.com/landelare/ue5coro/blob/be8ad2958221475ec48d8d66ec0512e4ade8b630/Docs/Async.md) can be
used in, making them a good place to create latent gameplay code. They are like mini states within a single machine
state, but when a label finishes executing its code nothing happens automatically afterwards. Labels either have to
manage themselves so that they carry on the logic invocation, or something else, like state functions, has to do it; it
depends on the context.

## Setup

Every label must follow a certain signature, and each one of them must be associated with a gameplay tag that also
must follow a template. To bind the two you must call `REGISTER_LABEL()` macro within the state's constructor.

```c++
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateMachine_Label_MyCoolLabel, "StateMachine.Label.MyCoolLabel");

UMyMachineState::UMyMachineState()
{
	REGISTER_LABEL(MyCoolLabel);
	// Other labels...
}

TCoroutine<> UMyMachineState::Label_MyCoolLabel()
{
	// Something...
}
```

## Starting label

The machine state comes with a default label called Default which is executed when the state starts executing if not
told otherwise. There are different ways of telling what label the state must start with, but we'll be covering the
easiest ones.

### Client instigation

The client code that makes a state be executed can also tell the starting label gameplay tag. There are different
ways of doing this, and certain methods are applicable in very strict situations.

If you have the finite state machine or machine state object reference you can use that to call the following methods:

```c++
bool GotoState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default, bool bForceEvents = true);
TCoroutine<> PushState(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default, bool* bOutPrematureResult = nullptr);
TCoroutine<> PushStateQueued(TSubclassOf<UMachineState> InStateClass, FGameplayTag Label = TAG_StateMachine_Label_Default, FFSM_PushRequestHandle* OutHandle = nullptr);
```

In both the cases you can use the second argument to alter the starting label from the default one to any other.

There are some macros that users are **heavily encouraged** to use inside labels, as they cover some boilerplate that
might change with the plugin development. Using regular variants will result into undefined behavior.

```c++
GOTO_STATE_LABEL(STATE_NAME, LABEL_NAME, ...)
GOTO_STATE_CLASS_LABEL(STATE_CLASS, LABEL_NAME, ...)
PUSH_STATE_LABEL(STATE_NAME, LABEL_NAME)
PUSH_STATE_CLASS_LABEL(STATE_CLASS, LABEL_NAME)
PUSH_STATE_QUEUED_LABEL(STATE_NAME, LABEL_NAME)
PUSH_STATE_QUEUED_CLASS_LABEL(STATE_CLASS, LABEL_NAME)
```

Unlike the methods these are a bit trickier to use:

```c++
TCoroutine<> Label_Default()
{
	// If the GotoState will success this label is going to terminate immediately
	GOTO_STATE_LABEL(UMyMachineState_Demo, MyCoolLabel);
	GOTO_STATE_CLASS_LABEL(UMyMachineState_Demo::StaticClass(), MyCoolLabel);
	
	// The code is going to wait upon SUCCESSFULLY pushing a new state to the stack up until this state becomes the 
	// top-most one once again
	PUSH_STATE_LABEL(UMyMachineState_Demo, MyCoolLabel);
	PUSH_STATE_CLASS_LABEL(UMyMachineState_Demo::StaticClass(), MyCoolLabel);
	
	// The code is going to wait the push result, as long as the arguments were valid, and the pushed state's 
	// subsequent deactivation. It means that the state might've not been pushed immediately, but it might be at some 
	// point, for instance, after the active state is changed to something that doesn't prevent the request from executing
	PUSH_STATE_QUEUED_LABEL(UMyMachineState_Demo, MyCoolLabel);
	PUSH_STATE_QUEUED_CLASS_LABEL(UMyMachineState_Demo::StaticClass(), MyCoolLabel);
}
```

These can be used outside the labels, but in that case co_await is impossible as only labels support coroutines by
default. Creating your own non-label coroutines will result into undefined behavior since the plugin comes with a
`StopLatentExecution()` which stops ALL labels (even those running in other states).

### Internal switch

Since the states have different events upon certain actions (Begin, End, Push, Pop etc) it's possible to switch the 
label right upon that, and it'll make us to ignore the label the client might've specified. At the moment of OnActivated 
(the same applies to Begin, Push, and Resume) the label the client has specified will be already assigned to us in 
UMachineState::ActiveLabel, and we're going to override it with our own label. It's not the best choice, but sometimes 
it might be handy.

```c++
void UMyMachineState::OnActivated()
{
	Super::OnActivated();
	
	GOTO_LABEL(MyCoolLabel);
}
```

Like that the state will always start at `Label_MyCoolLabel()` rather than `Label_Default()`.

## Switching between labels

At some point you'll want to execute different labels to achieve more complex behavior. To switch between them
there are different methods depending on the context you're doing it under.

If you have the finite state machine or machine state object reference you can use that to call the following method:

```c++
bool GotoLabel(FGameplayTag Label);
```

There is a simple macro that users are **heavily encouraged** to use inside labels, as it covers some boilerplate that
might change with the plugin development. Using regular variants will result into undefined behavior.

```c++
GOTO_LABEL(LABEL_NAME)
```

Like other macros, all you have to do is specify the final bit of the gameplay tag like this

```c++
TCoroutine<> Label_Default()
{
	// If the GotoLabel will success this label is going to terminate immediately
	GOTO_LABEL(MyCoolLabel);
}
```

## Code execution

When working in labels users are able to use coroutines to create latent code. It's a powerful tool, but in order to
work correctly within C++ and the FSM along other features specific actions have to be done. Everything is wrapped
up with a single macro which users are **heavily encouraged** to use. Not using it will result into undefined behavior.

```c++
RUN_LATENT_EXECUTION(FUNCTION, ...)
```

The function **must** be [co_awaitable](https://github.com/landelare/ue5coro/blob/master/Docs/Awaiters.md). 
This is how you can make a simple label:

```c++
TCoroutine<> UPatrolState::Label_Default()
{
	while (true)
	{
		AActor* MovePoint = GetMovePoint();

		// Move to a point
		RUN_LATENT_EXECUTION(AI::AIMoveTo, MyController, MovePoint, -1.f, EAIOptionFlag::Disable);

		// Stay idle for 5-10 seconds
		RUN_LATENT_EXECUTION(Latent::Seconds, FMath::FRandRange(5.f, 10.f));
	}
}
```
