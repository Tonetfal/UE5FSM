// Author: Antonio Sidenko (Tonetfal). All rights reserved.

#pragma once

#include "UE5Coro.h"
#include "Tasks/AITask_MoveTo.h"

class UAITask_MoveTo;

namespace UE5Coro
{
	namespace UE5FSM::AI
	{
		/**
		 * Issues a "move to" command to the specified controller, resumes the awaiting
		 * coroutine once it finishes.<br>
		 * The result of the await expression is EPathFollowingResult.
		 */
		UE5FSM_ADDITIONALCOROUTINE_API auto AIMoveTo(
			AAIController* Controller, FVector Target, float AcceptanceRadius = -1,
			EAIOptionFlag::Type StopOnOverlap = EAIOptionFlag::Default,
			EAIOptionFlag::Type AcceptPartialPath = EAIOptionFlag::Default,
			bool bUsePathfinding = true, bool bLockAILogic = true,
			bool bUseContinuousGoalTracking = false,
			EAIOptionFlag::Type ProjectGoalOnNavigation = EAIOptionFlag::Default,
			EAIOptionFlag::Type RequireNavigableEndLocation = EAIOptionFlag::Default)
			-> TCoroutine<EPathFollowingResult::Type>;

		/**
		 * Issues a "move to" command to the specified controller, resumes the awaiting
		 * coroutine once it finishes.<br>
		 * The result of the await expression is EPathFollowingResult.
		 */
		UE5FSM_ADDITIONALCOROUTINE_API auto AIMoveTo(
			AAIController* Controller, AActor* Target, float AcceptanceRadius = -1,
			EAIOptionFlag::Type StopOnOverlap = EAIOptionFlag::Default,
			EAIOptionFlag::Type AcceptPartialPath = EAIOptionFlag::Default,
			bool bUsePathfinding = true, bool bLockAILogic = true,
			bool bUseContinuousGoalTracking = false,
			EAIOptionFlag::Type ProjectGoalOnNavigation = EAIOptionFlag::Default,
			EAIOptionFlag::Type RequireNavigableEndLocation = EAIOptionFlag::Default)
			-> TCoroutine<EPathFollowingResult::Type>;
	}
}
