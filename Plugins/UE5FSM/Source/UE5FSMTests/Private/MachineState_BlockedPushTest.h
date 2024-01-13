#pragma once

#include "MachineState_Test.h"

#include "MachineState_BlockedPushTest.generated.h"

UCLASS()
class UMachineState_BlockedPushTest
	: public UMachineState_Test
{
	GENERATED_BODY()

public:

};

UCLASS()
class UMachineState_BlockedPushTest5
	: public UMachineState_BlockedPushTest
{
	GENERATED_BODY()

public:
	UMachineState_BlockedPushTest5()
	{
	}
};

UCLASS()
class UMachineState_BlockedPushTest4
	: public UMachineState_BlockedPushTest
{
	GENERATED_BODY()

public:
	UMachineState_BlockedPushTest4()
	{
		StatesBlocklist.Add(UMachineState_BlockedPushTest5::StaticClass());
	}
};

UCLASS()
class UMachineState_BlockedPushTest2
	: public UMachineState_BlockedPushTest
{
	GENERATED_BODY()

public:
	UMachineState_BlockedPushTest2()
	{
		StatesBlocklist.Add(UMachineState_BlockedPushTest4::StaticClass());
		StatesBlocklist.Add(UMachineState_BlockedPushTest5::StaticClass());
	}
};

UCLASS()
class UMachineState_BlockedPushTest1
	: public UMachineState_BlockedPushTest
{
	GENERATED_BODY()

public:
	UMachineState_BlockedPushTest1()
	{
		StatesBlocklist.Add(UMachineState_BlockedPushTest2::StaticClass());
	}
};

UCLASS()
class UMachineState_BlockedPushTest3
	: public UMachineState_BlockedPushTest
{
	GENERATED_BODY()

public:
	UMachineState_BlockedPushTest3()
	{
		StatesBlocklist.Add(UMachineState_BlockedPushTest1::StaticClass());
		StatesBlocklist.Add(UMachineState_BlockedPushTest5::StaticClass());
	}
};
