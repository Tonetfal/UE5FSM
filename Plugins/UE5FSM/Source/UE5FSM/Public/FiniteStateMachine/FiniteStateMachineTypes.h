#pragma once

#include "Logging/LogMacros.h"
#include "Stats/Stats2.h"

UE5FSM_API DECLARE_STATS_GROUP(TEXT("Finite State Machine"), STATGROUP_FiniteStateMachine, STATCAT_Advanced);

UE5FSM_API DECLARE_LOG_CATEGORY_EXTERN(LogFiniteStateMachine, Warning, All);

// Enables more verbosity for log messages
// MY_MODULE.Build.cs - PublicDefinitions.Add("FSM_EXTREME_VERBOSITY");
// #define FSM_EXTREME_VERBOSITY
