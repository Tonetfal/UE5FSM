#include "MachineState_StatesBlocklistTest.h"

UMachineState_StatesBlocklistTest1::UMachineState_StatesBlocklistTest1()
{
	StatesBlocklist.Add(UMachineState_StatesBlocklistTest2::StaticClass());
}

UMachineState_StatesBlocklistTest2::UMachineState_StatesBlocklistTest2()
{
	StatesBlocklist.Add(UMachineState_StatesBlocklistTest3::StaticClass());
}

UMachineState_StatesBlocklistTest3::UMachineState_StatesBlocklistTest3()
{
	StatesBlocklist.Add(UMachineState_StatesBlocklistTest1::StaticClass());
}
