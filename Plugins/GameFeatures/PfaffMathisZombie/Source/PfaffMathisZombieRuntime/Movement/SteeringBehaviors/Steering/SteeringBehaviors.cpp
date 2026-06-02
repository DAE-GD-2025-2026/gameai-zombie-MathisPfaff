#include "SteeringBehaviors.h"
#include "GameAI_Zombie/Survivor/SurvivorPawn.h"
#include "DrawDebugHelpers.h"

// Helper — gets 2D position from pawn
static FVector2D GetPos(ASurvivorPawn& Agent)
{
	return FVector2D(Agent.GetActorLocation());
}

// Helper — gets 2D velocity from pawn
static FVector2D GetVel(ASurvivorPawn& Agent)
{
	return FVector2D(Agent.GetVelocity());
}

// Helper — gets max speed from FloatingPawnMovement
static float GetMaxSpeed(ASurvivorPawn& Agent)
{
	if (auto* Move = Agent.FindComponentByClass<UFloatingPawnMovement>())
		return Move->MaxSpeed;
	return 400.f;
}

//SEEK
SteeringOutput Seek::CalculateSteering(float DeltaT, ASurvivorPawn& Agent)
{
	SteeringOutput steering{};
	steering.LinearVelocity = Target.Position - GetPos(Agent);

#if ENABLE_DRAW_DEBUG
	DrawDebugPoint(Agent.GetWorld(), FVector(Target.Position, 0), 10.f, FColor::Red, false, -1);
	DrawDebugLine(Agent.GetWorld(), Agent.GetActorLocation(),
		Agent.GetActorLocation() + FVector(GetVel(Agent) / 3.f, 0), FColor::Green, false, -1);
#endif
	return steering;
}

//FLEE
SteeringOutput Flee::CalculateSteering(float DeltaT, ASurvivorPawn& Agent)
{
	SteeringOutput steering{};
	steering.LinearVelocity = -(Target.Position - GetPos(Agent));

#if ENABLE_DRAW_DEBUG
	DrawDebugPoint(Agent.GetWorld(), FVector(Target.Position, 0), 10.f, FColor::Red, false, -1);
#endif
	return steering;
}

//ARRIVE
SteeringOutput Arrive::CalculateSteering(float DeltaT, ASurvivorPawn& Agent)
{
	SteeringOutput steering{};

	steering.LinearVelocity = Target.Position - GetPos(Agent);
	float dist = steering.LinearVelocity.Length();

	if (dist < TargetRadius)
		SpeedScale = 0.f;
	else if (dist < SlowRadius)
		SpeedScale = dist / SlowRadius;
	else
		SpeedScale = 1.f;

#if ENABLE_DRAW_DEBUG
	DrawDebugPoint(Agent.GetWorld(), FVector(Target.Position, 0), 10.f, FColor::Red, false, -1);
#endif
	return steering;
}

//FACE
SteeringOutput Face::CalculateSteering(float DeltaT, ASurvivorPawn& Agent)
{
	SteeringOutput steering{};

	FVector2D toTarget = Target.Position - FVector2D(Agent.GetActorLocation());
	if (toTarget.IsNearlyZero()) return steering;

	float desiredAngle = FMath::RadiansToDegrees(FMath::Atan2(toTarget.Y, toTarget.X));
	float currentAngle = Agent.GetActorRotation().Yaw;
	steering.AngularVelocity = FMath::FindDeltaAngleDegrees(currentAngle, desiredAngle) / 180.f;

	return steering;
}

//PURSUIT
SteeringOutput Pursuit::CalculateSteering(float DeltaT, ASurvivorPawn& Agent)
{
	SteeringOutput steering{};

	FVector2D toTarget     = Target.Position - FVector2D(Agent.GetActorLocation());
	float timeToTarget     = toTarget.Length() / FMath::Max(GetMaxSpeed(Agent), 1.f);
	FVector2D predicted    = Target.Position + Target.LinearVelocity * timeToTarget;
	steering.LinearVelocity = predicted - FVector2D(Agent.GetActorLocation());

#if ENABLE_DRAW_DEBUG
	DrawDebugPoint(Agent.GetWorld(), FVector(predicted, 0), 10.f, FColor::Red, false, -1);
#endif
	return steering;
}

//EVADE
SteeringOutput Evade::CalculateSteering(float DeltaT, ASurvivorPawn& Agent)
{
	SteeringOutput steering{};
	constexpr float evadeDistance = 800.f;

	FVector2D AgentPos  = FVector2D(Agent.GetActorLocation());
	FVector2D toTarget  = Target.Position - AgentPos;

	// Use combined speeds for more accurate prediction
	float pursuerSpeed    = Target.LinearVelocity.Length();
	float mySpeed         = FMath::Max(GetMaxSpeed(Agent), 1.f);
	float combinedSpeed   = mySpeed + (pursuerSpeed > 0.f ? pursuerSpeed : mySpeed);
	float timeToTarget    = toTarget.Length() / combinedSpeed;

	FVector2D predicted   = Target.Position + Target.LinearVelocity * timeToTarget;
	FVector2D toPredicted = predicted - AgentPos;

	if (toPredicted.Length() < evadeDistance)
	{
		// Flee from predicted position (true evade)
		steering.LinearVelocity = AgentPos - predicted;
	}
	else
	{
		// Outside evade range — still flee from current zombie position so we never stand still
		steering.LinearVelocity = AgentPos - Target.Position;
	}

#if ENABLE_DRAW_DEBUG
	DrawDebugCircle(Agent.GetWorld(), Agent.GetActorLocation(), evadeDistance, 16,
		FColor::Orange, false, -1.f, 0, 2.f, FVector(0,1,0), FVector(1,0,0), false);
	DrawDebugPoint(Agent.GetWorld(), FVector(predicted, 0), 10.f, FColor::Red, false, -1);
	DrawDebugLine(Agent.GetWorld(), Agent.GetActorLocation(),
		Agent.GetActorLocation() + FVector(FVector2D(Agent.GetVelocity()) / 3.f, 0),
		FColor::Green, false, -1);
#endif

	return steering;
}

//WANDER
SteeringOutput Wander::CalculateSteering(float DeltaT, ASurvivorPawn& Agent)
{
	SteeringOutput steering{};

	m_WanderAngle += FMath::RandRange(-m_MaxAngleChange, m_MaxAngleChange);
	m_WanderAngle  = FMath::Clamp(m_WanderAngle, -PI, PI);

	FVector2D fwd         = FVector2D(Agent.GetActorForwardVector());
	FVector2D circleCenter = FVector2D(Agent.GetActorLocation()) + fwd * m_OffsetDistance;
	FVector2D pointOnCircle {
		circleCenter.X + FMath::Cos(m_WanderAngle) * m_Radius,
		circleCenter.Y + FMath::Sin(m_WanderAngle) * m_Radius
	};

	steering.LinearVelocity = pointOnCircle - FVector2D(Agent.GetActorLocation());

#if ENABLE_DRAW_DEBUG
	DrawDebugCircle(Agent.GetWorld(), FVector(circleCenter, 0), m_Radius, 32,
		FColor::Yellow, false, -1.f, 0, 2.f, FVector(0,1,0), FVector(1,0,0), false);
	DrawDebugPoint(Agent.GetWorld(), FVector(pointOnCircle, 0), 10.f, FColor::Red, false, -1);
#endif
	return steering;
}