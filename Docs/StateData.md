# State data

## Description

State data is a simple object that is contained by a particular state. It's nothing but an empty UObject users are 
meant to fill up with data relative to a state for their own purposes. The object is accessible from outside using the 
owning FSM to make it easy to expose information to anything, for instance to other states as they might base their
behavior on it, or pass some information to us.

## Setup

To create your own state data object override `UMachineStateData`. It might look something like this:

```c++
UCLASS()
class UMyStateData : public UMachineStateData
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<AActor> Target = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float AttackRadius = 50.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	bool bDamageInterruptsAttacking = true;
};
```

Most likely you're going to edit certain properties within BPs, so you would need to inherit your state data, and then
tell a particular state to use the BP version of it instead of the C++ one which doesn't have the edited values.

To change the default state data to class a particular state is going to use there's one property users are meant to 
modify:

```c++
UPROPERTY(EditDefaultsOnly, Category="Data")
TSubclassOf<UMachineStateData> StateDataClass = nullptr;
```

The class must be modified before the machine state is initialized. Generally you should do that either inside the 
machine state constructor or from the BPs properties.

## Initialization

If you want to add any custom logic to machine state initialization you're presented with the following methods:

```c++
void UMachineState::Initialize();
UMachineStateData* UMachineState::CreateStateData();
```

The data object is created once the state is initialized, and stays there until the end of the state life cycle.

## Accessing the data

There are different ways to access the state data depending on the context. 

When working within a state you have the following property:

```c++
UPROPERTY()
TObjectPtr<UMachineStateData> BaseStateData = nullptr;
```

However since `UMachineStateData` is empty, you're meant to downcast it to get your specific state data, and you 
have to store it in your state to avoid casting it everytime you need to access it. You can do that within the 
aforementioned `UMachineState::CreateStateData()`.

When you need to access data of a particular state outside it, you can use the following methods:

```c++
template<typename StateDataClass, typename StateClass>
StateDataClass* UFiniteStateMachine::GetStateData() const;

template<typename StateDataClass, typename StateClass>
StateDataClass* UFiniteStateMachine::GetStateDataChecked() const;
```

You can use these methods like this:

```c++
auto* MyStateData = StateMachine->GetStateData<UMyStateData, UMyMachineState>();
if (IsValid(MyStateData))
{
	// Something...
}
```

```c++
// If the game doesn't crash, the MyStateData *will* be valid, so avoid checking with IsValid
auto* MyStateData = StateMachine->GetStateDataChecked<UMyStateData, UMyMachineState>();
// Something...
```
