#pragma once

#include <vector>

#include "../Steering/SteeringBehaviors.h"

class PathFollow : public ISteeringBehavior
{
public:
	PathFollow();
	virtual ~PathFollow() override;
	void SetPath(const TArray<FVector>& path);
	virtual SteeringOutput CalculateSteering(float DeltaTime, ASurvivorPawn& Agent) override;

private:
	Seek* pSeek = nullptr;
	Arrive* pArrive = nullptr;
	ISteeringBehavior* pCurrentSteering = nullptr;
	TArray<FVector> pathVec = {};
	int currentPathIndex = 0;

	void GotoNextPathPoint();
};