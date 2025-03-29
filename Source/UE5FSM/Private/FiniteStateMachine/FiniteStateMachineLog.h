#pragma once

#include "FiniteStateMachine/FiniteStateMachineTypes.h"

#ifdef FSM_EXTREME_VERBOSITY
	#define FSM_EXTEREME_VERBOSITY_STR FString::Printf(TEXT("Owner [%s] - "), *GetNameSafe(GetOwner()))
#else
	#define FSM_EXTEREME_VERBOSITY_STR FString(TEXT(""))
#endif

#define FSM_LOG(VERBOSITY, MESSAGE, ...) UE_LOG(LogFiniteStateMachine, VERBOSITY, TEXT("%s%s"), \
	*FSM_EXTEREME_VERBOSITY_STR, *FString::Printf(TEXT(MESSAGE), ## __VA_ARGS__))
