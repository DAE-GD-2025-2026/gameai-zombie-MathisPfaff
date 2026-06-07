#include "PathFollowSteeringBehaviorPfaffMathis.h"
#include "GameAI_Zombie/Survivor/SurvivorPawn.h"

PathFollowPfaffMathis::PathFollowPfaffMathis()
{
	pSeek = new SeekPfaffMathis();
	pArrive = new ArrivePfaffMathis();
	pArrive->SetTargetRadius(10.0f);
}

PathFollowPfaffMathis::~PathFollowPfaffMathis()
{
	delete pArrive;
	delete pSeek;
}

void PathFollowPfaffMathis::SetPath(const TArray<FVector>& path)
{
	pathVec = path;
	currentPathIndex = -1;
	GotoNextPathPoint();
}

SteeringOutput PathFollowPfaffMathis::CalculateSteering(float DeltaTime, ASurvivorPawn& Agent)
{
	if (currentPathIndex < pathVec.Num())
	{
		constexpr float ArrivalThreshold = 50.f;
		FVector2D ToPathPoint{ FVector2D(pathVec[currentPathIndex]) - FVector2D(Agent.GetActorLocation()) };

		if (ToPathPoint.SizeSquared() < ArrivalThreshold * ArrivalThreshold)
			GotoNextPathPoint();
	}

	if (pCurrentSteering != nullptr)
		return pCurrentSteering->CalculateSteering(DeltaTime, Agent);

	return SteeringOutput{};
}

void PathFollowPfaffMathis::GotoNextPathPoint()
{
	++currentPathIndex;
	if (currentPathIndex >= pathVec.Num()) return;

	FTargetData PathTarget{ FVector2D(pathVec[currentPathIndex]) };

	if (currentPathIndex == pathVec.Num() - 1)
	{
		pArrive->SetTarget(PathTarget);
		pCurrentSteering = pArrive;
	}
	else
	{
		pSeek->SetTarget(PathTarget);
		pCurrentSteering = pSeek;
	}
}