# Global State

## Description

Global state serves as a supervisor for the FSM. It's intended to share its data with other states, and this can be
thought about as a Blackboard. Also it might execute some arbitrary code regardless of the current state, for
instance it might update the aggro or determine what target the AI should be attacking. Unlike normal states these
cannot be switched on runtime. They can be either present or not, all you have to do is either define the global
state class in the FSM or leave it as none.

Syntactically the global state is not really different from a normal state, currently the only difference is that it
implements `IGlobalMachineStateInterface` to differentiate it from others. With the plugin development it might obtain
some unique features, but for now it's just an empty interface to use within `TSubclassOf` meta properties to filter out
normal states.

Some of the features from [normal states](NormalState.md) apply to this state as well. The ones that do **not** apply:
- [Registration](NormalState.md#registration): Global states are automatically registered.
- [Initial FSM state](NormalState.md#initial-fsm-state): If global state is present, it'll always be activated upon FSM 
  initialization.
- [Stack](NormalState.md#stack): Global state is **not** present on the stack, hence not altered by pushing, popping, 
  going to state etc, however it gets the Begin and End events.

## Initialization

Upon implementing your own global machine state you have to tell the FSM to should be using it using the following
method:

```c++
void UFiniteStateMachine::SetGlobalState(TSubclassOf<UMachineState> InStateClass);
```

Global state can be selected **only** during FSM initialization.

A better way of doing that is by manually assigning it to the FSM within BPs. If the FSM is under an AI controller,
you're encouraged to edit the following property:

```c++
TSubclassOf<UMachineState> UFiniteStateMachine::GlobalStateClass = nullptr;
```  

The plugin does **not** come with a default global state as users are meant to override their custom normal states,
and inherit the `IGlobalMachineStateInterface` interface.

## Tips

- override the default [state data](StateData.md#setup) class (`UMachineState::StateDataClass`) used by the global state 
  in order to have a common storage.
- create the global state and its data in C++, and then override it in BPs to ease the properties editing. Make sure
  to assign the correct classes in the state and in the FSM.