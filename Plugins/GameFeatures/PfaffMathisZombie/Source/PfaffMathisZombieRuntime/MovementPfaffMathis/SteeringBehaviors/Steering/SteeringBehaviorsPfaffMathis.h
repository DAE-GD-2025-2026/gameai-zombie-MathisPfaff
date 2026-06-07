#pragma once

#include "MovementPfaffMathis/SteeringBehaviors/SteeringHelpersPfaffMathis.h"
#include "Kismet/KismetMathLibrary.h"

class ASurvivorPawn;

class ISteeringBehavior
{
public:
	ISteeringBehavior() = default;
	virtual ~ISteeringBehavior() = default;

	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) = 0;

	void SetTarget(const FTargetData& NewTarget) { Target = NewTarget; }

	template<class T, std::enable_if_t<std::is_base_of_v<ISteeringBehavior, T>>* = nullptr>
	T* As() { return static_cast<T*>(this); }

protected:
	FTargetData Target;
};

class Seek : public ISteeringBehavior
{
public:
	Seek() = default;
	virtual ~Seek() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class Flee : public ISteeringBehavior
{
public:
	Flee() = default;
	virtual ~Flee() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class Arrive : public ISteeringBehavior
{
public:
	Arrive() = default;
	virtual ~Arrive() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;

	void SetTargetRadius(float radius) { TargetRadius = radius; }
	void SetSlowRadius(float radius)   { SlowRadius = radius; }
	float GetSpeedScale() const { return SpeedScale; }

private:
	float SlowRadius  { 500.f };
	float TargetRadius{ 100.f };
	float SpeedScale  { 1.f   };
};

class Face : public ISteeringBehavior
{
public:
	Face() = default;
	virtual ~Face() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class Pursuit : public ISteeringBehavior
{
public:
	Pursuit() = default;
	virtual ~Pursuit() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class Evade : public ISteeringBehavior
{
public:
	Evade() = default;
	virtual ~Evade() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class Wander : public Seek
{
public:
	Wander() = default;
	virtual ~Wander() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;

	void SetWanderOffset(float offset)   { m_OffsetDistance = offset; }
	void SetWanderRadius(float radius)   { m_Radius = radius; }
	void SetMaxAngleChange(float rad)    { m_MaxAngleChange = rad; }

protected:
	float m_OffsetDistance { 200.f };
	float m_Radius         { 100.f };
	float m_MaxAngleChange { FMath::DegreesToRadians(20.f) };
	float m_WanderAngle    { 0.f   };
};