#pragma once

#include "Logging/LogMacros.h"
#include "Stats/Stats2.h"

FINITESTATEMACHINEMODULE_API DECLARE_STATS_GROUP(TEXT("Finite State Machine"), STATGROUP_FiniteStateMachine, STATCAT_Advanced);

FINITESTATEMACHINEMODULE_API DECLARE_LOG_CATEGORY_EXTERN(LogFiniteStateMachine, Warning, All);

// Enables more verbosity for log messages
// #define FSM_EXTEREME_VERBOSITY
