#pragma once
#include <vector>

#include "MovementPfaffMathis/SteeringBehaviorsPfaffMathis/SteeringPfaffMathis/SteeringBehaviorsPfaffMathis.h"

//****************
//BLENDED STEERING
class BlendedSteeringPfaffMathis final: public ISteeringBehaviorPfaffMathis
{
public:
	struct WeightedBehavior
	{
		ISteeringBehaviorPfaffMathis* pBehavior = nullptr;
		float Weight = 0.f;

		WeightedBehavior(ISteeringBehaviorPfaffMathis* const pBehavior, float Weight) :
			pBehavior(pBehavior),
			Weight(Weight)
		{};
	};

	BlendedSteeringPfaffMathis(const std::vector<WeightedBehavior>& WeightedBehaviors);

	void AddBehaviour(const WeightedBehavior& WeightedBehavior) { WeightedBehaviors.push_back(WeightedBehavior); }
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;

	float* GetWeight(ISteeringBehaviorPfaffMathis* const SteeringBehavior);
	
	// returns a reference to the weighted behaviors, can be used to adjust weighting. Is not intended to alter the behaviors themselves.
	std::vector<WeightedBehavior>& GetWeightedBehaviorsRef() { return WeightedBehaviors; }

private:
	std::vector<WeightedBehavior> WeightedBehaviors = {};

	//using ISteeringBehavior::SetTarget; // made private because targets need to be set on the individual behaviors, not the combined behavior
};

//*****************
//PRIORITY STEERING
class PrioritySteeringPfaffMathis final: public ISteeringBehaviorPfaffMathis
{
public:
	PrioritySteeringPfaffMathis(const std::vector<ISteeringBehaviorPfaffMathis*>& priorityBehaviors)
		:m_PriorityBehaviors(priorityBehaviors) 
	{}

	void AddBehaviour(ISteeringBehaviorPfaffMathis* const pBehavior) { m_PriorityBehaviors.push_back(pBehavior); }
	SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;

private:
	std::vector<ISteeringBehaviorPfaffMathis*> m_PriorityBehaviors = {};

	// using ISteeringBehavior::SetTarget; // made private because targets need to be set on the individual behaviors, not the combined behavior
};