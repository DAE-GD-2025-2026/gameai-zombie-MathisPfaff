#pragma once

#include "Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include <functional>
#include <vector>
#include <string>

class ASurvivorPawn;

// ===========================================================================
// Curve helpers
// ===========================================================================
enum class ECurveType : uint8_t
{
    Linear,
    InverseLinear,
    SmoothStep,
    SineCurve,
    Cosine,
    Exponential,
    Logarithmic,
};

inline const char* CurveTypeName(ECurveType T)
{
    switch (T) {
    case ECurveType::Linear:        return "Linear";
    case ECurveType::InverseLinear: return "InverseLinear";
    case ECurveType::SmoothStep:    return "SmoothStep";
    case ECurveType::SineCurve:     return "SineCurve";
    case ECurveType::Cosine:        return "Cosine";
    case ECurveType::Exponential:   return "Exponential";
    case ECurveType::Logarithmic:   return "Logarithmic";
    default:                        return "Unknown";
    }
}

inline float EvaluateCurve(ECurveType T, float x)
{
    float t = FMath::Clamp(x, 0.f, 1.f);
    switch (T) {
    case ECurveType::Linear:        return t;
    case ECurveType::InverseLinear: return 1.f - t;
    case ECurveType::SmoothStep:    return t * t * (3.f - 2.f * t);
    case ECurveType::SineCurve:     return (1.f - FMath::Cos(t * PI)) * 0.5f;
    case ECurveType::Cosine:        return FMath::Sin(t * PI * 0.5f);
    case ECurveType::Exponential:   return FMath::Pow(t, 2.f);
    case ECurveType::Logarithmic:   return FMath::Clamp(FMath::Loge(1.f + t * 9.f) / FMath::Loge(10.f), 0.f, 1.f);
    default:                        return t;
    }
}

// ===========================================================================
// Consideration
// ===========================================================================
template<typename TContext>
struct TConsideration
{
    std::string                           Name;
    std::function<float(const TContext&)> GetRawValue;
    ECurveType                            CurveType = ECurveType::Linear;

    float Evaluate(const TContext& Ctx) const
    {
        return EvaluateCurve(CurveType, GetRawValue(Ctx));
    }
};

// ===========================================================================
// Base action
// ===========================================================================
class IUtilityAction
{
public:
    std::string Name;
    virtual ~IUtilityAction() = default;

    virtual float              Score()                        const = 0;
    virtual void               Execute(ASurvivorPawn&, float)       = 0;
    virtual void               OnEnter(ASurvivorPawn&)              = 0;
    virtual void               OnExit (ASurvivorPawn&)              = 0;
    virtual ISteeringBehavior* GetActiveBehavior()                  = 0;
};

template<typename TContext>
static float CalcScore(const std::vector<TConsideration<TContext>>& Considerations,
                       const TContext& Ctx)
{
    if (Considerations.empty()) return 0.f;
    float total = 1.f;
    for (auto& c : Considerations)
        total *= c.Evaluate(Ctx);
    float mod = 1.f - (1.f / static_cast<float>(Considerations.size()));
    return FMath::Clamp(total + total * mod * (1.f - total), 0.f, 1.f);
}

// ===========================================================================
// EVADE ZOMBIE ACTION
// Context: how close the nearest zombie is (0=far, 1=touching)
//   Zombie close → evade scores high
// ===========================================================================
struct FEvadeZombieContext
{
    float NormalizedProximityToZombie = 0.f; // you set this every tick
};

class UAEvadeZombieAction : public IUtilityAction
{
public:
    FEvadeZombieContext                              Context;
    std::vector<TConsideration<FEvadeZombieContext>> Considerations;

    UAEvadeZombieAction()
    {
        Name = "EvadeZombie";

        Considerations.push_back({
            "ZombieClose",
            [](const FEvadeZombieContext& c){ return c.NormalizedProximityToZombie; },
            ECurveType::Exponential  // spikes hard when zombie is very close
        });
    }

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn&) override {}
    void OnExit (ASurvivorPawn&) override {}
    void Execute(ASurvivorPawn&, float)   override {}

    ISteeringBehavior* GetActiveBehavior() override { return &m_Evade; }

    void SetTarget(const FTargetData& T) { m_Evade.SetTarget(T); }

private:
    Evade m_Evade;
};