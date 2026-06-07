#pragma once

#include <vector>

#include "../SteeringPfaffMathis/SteeringBehaviorsPfaffMathis.h"

class PathFollowPfaffMathis : public ISteeringBehaviorPfaffMathis
{
public:
	PathFollowPfaffMathis();
	virtual ~PathFollowPfaffMathis() override;
	void SetPath(const TArray<FVector>& path);
	virtual SteeringOutput CalculateSteering(float DeltaTime, ASurvivorPawn& Agent) override;

private:
	SeekPfaffMathis* pSeek = nullptr;
	ArrivePfaffMathis* pArrive = nullptr;
	ISteeringBehaviorPfaffMathis* pCurrentSteering = nullptr;
	TArray<FVector> pathVec = {};
	int currentPathIndex = 0;

	void GotoNextPathPoint();
};