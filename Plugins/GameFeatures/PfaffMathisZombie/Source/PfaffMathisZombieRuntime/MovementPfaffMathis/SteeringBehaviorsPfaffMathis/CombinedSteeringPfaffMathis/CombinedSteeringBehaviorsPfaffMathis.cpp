
#include "CombinedSteeringBehaviorsPfaffMathis.h"
#include <algorithm>
#include "GameAI_Zombie/Survivor/SurvivorPawn.h"

BlendedSteeringPfaffMathis::BlendedSteeringPfaffMathis(const std::vector<WeightedBehavior>& WeightedBehaviors)
	:WeightedBehaviors(WeightedBehaviors)
{};

//****************
//BLENDED STEERING
SteeringOutput BlendedSteeringPfaffMathis::CalculateSteering(float DeltaT, ASurvivorPawn& Agent)
{
	SteeringOutput BlendedSteering = {};
	float TotalWeight = 0.f;

	for (auto& WeightedBehavior : WeightedBehaviors)
	{
		if (WeightedBehavior.pBehavior && WeightedBehavior.Weight > 0.f)
		{
			SteeringOutput BehaviorOutput = WeightedBehavior.pBehavior->CalculateSteering(DeltaT, Agent);

			if (!BehaviorOutput.LinearVelocity.IsNearlyZero())
			{
				BehaviorOutput.LinearVelocity.Normalize();
			}

			BlendedSteering.LinearVelocity  += BehaviorOutput.LinearVelocity  * WeightedBehavior.Weight;
			BlendedSteering.AngularVelocity += BehaviorOutput.AngularVelocity * WeightedBehavior.Weight;
			TotalWeight += WeightedBehavior.Weight;
		}
	}

	if (TotalWeight > 0.f)
	{
		BlendedSteering.LinearVelocity *= 1.f / TotalWeight;
	}
	
	// TODO: Add debug drawing

	return BlendedSteering;
}

float* BlendedSteeringPfaffMathis::GetWeight(ISteeringBehaviorPfaffMathis* const SteeringBehavior)
{
	auto it = find_if(WeightedBehaviors.begin(),
		WeightedBehaviors.end(),
		[SteeringBehavior](const WeightedBehavior& Elem)
		{
			return Elem.pBehavior == SteeringBehavior;
		}
	);

	if(it!= WeightedBehaviors.end())
		return &it->Weight;
	
	return nullptr;
}

//*****************
//PRIORITY STEERING
SteeringOutput PrioritySteeringPfaffMathis::CalculateSteering(float DeltaT, ASurvivorPawn& Agent)
{
	SteeringOutput Steering = {};

	for (ISteeringBehaviorPfaffMathis* const pBehavior : m_PriorityBehaviors)
	{
		Steering = pBehavior->CalculateSteering(DeltaT, Agent);

		if (!Steering.LinearVelocity.IsNearlyZero())
		{
			break;
		}
	}

	return Steering;
}