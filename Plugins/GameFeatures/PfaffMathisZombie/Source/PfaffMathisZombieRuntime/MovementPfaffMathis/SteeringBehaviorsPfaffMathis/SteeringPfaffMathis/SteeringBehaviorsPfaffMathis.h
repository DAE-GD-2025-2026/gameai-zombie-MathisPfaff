#pragma once

#include "MovementPfaffMathis/SteeringBehaviorsPfaffMathis/SteeringHelpersPfaffMathis.h"
#include "Kismet/KismetMathLibrary.h"

class ASurvivorPawn;

class ISteeringBehaviorPfaffMathis
{
public:
	ISteeringBehaviorPfaffMathis() = default;
	virtual ~ISteeringBehaviorPfaffMathis() = default;

	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) = 0;

	void SetTarget(const FTargetData& NewTarget) { Target = NewTarget; }

	template<class T, std::enable_if_t<std::is_base_of_v<ISteeringBehaviorPfaffMathis, T>>* = nullptr>
	T* As() { return static_cast<T*>(this); }

protected:
	FTargetData Target;
};

class SeekPfaffMathis : public ISteeringBehaviorPfaffMathis
{
public:
	SeekPfaffMathis() = default;
	virtual ~SeekPfaffMathis() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class FleePfaffMathis : public ISteeringBehaviorPfaffMathis
{
public:
	FleePfaffMathis() = default;
	virtual ~FleePfaffMathis() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class ArrivePfaffMathis : public ISteeringBehaviorPfaffMathis
{
public:
	ArrivePfaffMathis() = default;
	virtual ~ArrivePfaffMathis() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;

	void SetTargetRadius(float radius) { TargetRadius = radius; }
	void SetSlowRadius(float radius)   { SlowRadius = radius; }
	float GetSpeedScale() const { return SpeedScale; }

private:
	float SlowRadius  { 500.f };
	float TargetRadius{ 100.f };
	float SpeedScale  { 1.f   };
};

class FacePfaffMathis : public ISteeringBehaviorPfaffMathis
{
public:
	FacePfaffMathis() = default;
	virtual ~FacePfaffMathis() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class PursuitPfaffMathis : public ISteeringBehaviorPfaffMathis
{
public:
	PursuitPfaffMathis() = default;
	virtual ~PursuitPfaffMathis() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class EvadePfaffMathis : public ISteeringBehaviorPfaffMathis
{
public:
	EvadePfaffMathis() = default;
	virtual ~EvadePfaffMathis() override = default;
	virtual SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override;
};

class WanderPfaffMathis : public SeekPfaffMathis
{
public:
	WanderPfaffMathis() = default;
	virtual ~WanderPfaffMathis() override = default;
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