// Author: Antonio Sidenko (Tonetfal). All rights reserved.

#include "AICoroutines.h"

#include "AIController.h"
#include "UE5CoroAI.h"

namespace
{
	template<typename T>
	concept TGoal = std::same_as<T, FVector> || std::same_as<T, AActor*>;

	consteval bool IsValid(const FVector&)
	{
		return true;
	}
}

using namespace UE5Coro;

Private::FMoveToAwaiter AIMoveToCore(
	UAITask_MoveTo*& MoveToTask,
	AAIController* Controller, TGoal auto Target,
	float AcceptanceRadius,
	EAIOptionFlag::Type StopOnOverlap,
	EAIOptionFlag::Type AcceptPartialPath,
	bool bUsePathfinding, bool bLockAILogic,
	bool bUseContinuousGoalTracking,
	EAIOptionFlag::Type ProjectGoalOnNavigation,
	EAIOptionFlag::Type RequireNavigableEndLocation)
{
	checkf(IsInGameThread(),
		TEXT("This function may only be called from the game thread"));
	checkf(IsValid(Controller), TEXT("Attempting to move invalid controller"));
	checkf(IsValid(Target), TEXT("Attempting to move to invalid target"));
#if ENABLE_NAN_DIAGNOSTIC
	if (FMath::IsNaN(AcceptanceRadius))
	{
		logOrEnsureNanError(TEXT("AsyncMoveTo started with NaN radius"));
	}
	if constexpr (std::same_as<decltype(Target), FVector>)
		if (Target.ContainsNaN())
		{
			logOrEnsureNanError(TEXT("AsyncMoveTo started with NaN target"));
		}
#endif

	FVector Vector(ForceInit);
	AActor* Actor = nullptr;
	if constexpr (std::same_as<decltype(Target), FVector>)
		Vector = Target;
	else
		Actor = Target;

	MoveToTask = UAITask_MoveTo::AIMoveTo(
		Controller, Vector, Actor, AcceptanceRadius, StopOnOverlap,
		AcceptPartialPath, bUsePathfinding, bLockAILogic,
		bUseContinuousGoalTracking, ProjectGoalOnNavigation, RequireNavigableEndLocation);

	return Private::FMoveToAwaiter(MoveToTask);
}

TCoroutine<EPathFollowingResult::Type> UE5Coro::UE5FSM::AI::AIMoveTo(
	AAIController* Controller, FVector Target, float AcceptanceRadius,
	EAIOptionFlag::Type StopOnOverlap, EAIOptionFlag::Type AcceptPartialPath,
	bool bUsePathfinding, bool bLockAILogic, bool bUseContinuousGoalTracking,
	EAIOptionFlag::Type ProjectGoalOnNavigation, EAIOptionFlag::Type RequireNavigableEndLocation)
{
	UAITask_MoveTo* MoveToTask;
	auto Awaiter = AIMoveToCore(OUT MoveToTask, Controller, Target,
		AcceptanceRadius, StopOnOverlap, AcceptPartialPath, bUsePathfinding, bLockAILogic,
		bUseContinuousGoalTracking, ProjectGoalOnNavigation, RequireNavigableEndLocation);

	::FOnCoroutineCanceled _([WeakTask = TWeakObjectPtr<UAITask_MoveTo>(MoveToTask)]
	{
		if (auto* Task = WeakTask.Get())
		{
			Task->ExternalCancel();
		}
	});

	auto Result = co_await Awaiter;
	co_return Result;
}

TCoroutine<EPathFollowingResult::Type> UE5Coro::UE5FSM::AI::AIMoveTo(
	AAIController* Controller, AActor* Target, float AcceptanceRadius,
	EAIOptionFlag::Type StopOnOverlap, EAIOptionFlag::Type AcceptPartialPath,
	bool bUsePathfinding, bool bLockAILogic, bool bUseContinuousGoalTracking,
	EAIOptionFlag::Type ProjectGoalOnNavigation, EAIOptionFlag::Type RequireNavigableEndLocation)
{
	UAITask_MoveTo* MoveToTask;
	auto Awaiter = AIMoveToCore(OUT MoveToTask, Controller, Target,
		AcceptanceRadius, StopOnOverlap, AcceptPartialPath, bUsePathfinding, bLockAILogic,
		bUseContinuousGoalTracking, ProjectGoalOnNavigation, RequireNavigableEndLocation);

	::FOnCoroutineCanceled _([WeakTask = TWeakObjectPtr<UAITask_MoveTo>(MoveToTask)]
	{
		if (auto* Task = WeakTask.Get())
		{
			Task->ExternalCancel();
		}
	});

	auto Result = co_await Awaiter;
	co_return Result;
}
