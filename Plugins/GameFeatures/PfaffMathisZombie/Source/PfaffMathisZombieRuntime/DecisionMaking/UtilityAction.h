#pragma once

#include "Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include <functional>
#include <vector>
#include <string>
#include "GameAI_Zombie/Items/BaseItem.h"
#include "GameAI_Zombie/Items/ItemType.h"
#include "GameAI_Zombie/Common/HealthComponent.h"
#include "GameAI_Zombie/Items/Food.h"
#include "GameAI_Zombie/Common/InventoryComponent.h"
#include "EngineUtils.h"
#include "GameAI_Zombie/Village/House/House.h"
#include "Movement/SteeringBehaviors/PathFollow/PathFollowSteeringBehavior.h"

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
    float NormalizedProximityToZombie = 0.f;
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
            ECurveType::Exponential
        });
    }

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn& Agent) override { Agent.StartRunning(); }
    void OnExit (ASurvivorPawn& Agent) override { Agent.StopRunning(); }
    void Execute(ASurvivorPawn&, float) override {}

    ISteeringBehavior* GetActiveBehavior() override { return &m_Evade; }

    void SetTarget(const FTargetData& T) { m_Evade.SetTarget(T); }

private:
    Evade m_Evade;
};

// ===========================================================================
// SEEK MEDKIT ACTION
// Context: health ratio (0=dead, 1=full), whether a medkit is visible
// High urgency when health ≤ 30% and a medkit is perceived
// ===========================================================================
struct FSeekMedkitContext
{
    float NormalizedHealth    = 1.f;
    bool  bMedkitVisible      = false;
};

class UASeekMedkitAction : public IUtilityAction
{
public:
    FSeekMedkitContext                              Context;
    std::vector<TConsideration<FSeekMedkitContext>> Considerations;

    UASeekMedkitAction()
    {
        Name = "SeekMedkit";

        Considerations.push_back({
            "LowHealth",
            [](const FSeekMedkitContext& c){ return 1.f - c.NormalizedHealth; },
            ECurveType::Exponential
        });
        Considerations.push_back({
            "MedkitVisible",
            [](const FSeekMedkitContext& c){ return c.bMedkitVisible ? 1.f : 0.f; },
            ECurveType::Linear
        });
    }

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn&) override {}
    void OnExit (ASurvivorPawn&) override {}
    void Execute(ASurvivorPawn&, float)   override {}

    ISteeringBehavior* GetActiveBehavior() override { return &m_Arrive; }
    void SetTarget(const FTargetData& T) { m_Arrive.SetTarget(T); }

private:
    Arrive m_Arrive;
};

// ===========================================================================
// SEEK WEAPON ACTION
// Context: whether survivor has a weapon, whether a weapon is visible
// Medium priority — pick up a weapon if seen and unarmed
// ===========================================================================
struct FSeekWeaponContext
{
    bool bHasWeapon     = false;
    bool bWeaponVisible = false;
};

class UASeekWeaponAction : public IUtilityAction
{
public:
    FSeekWeaponContext                              Context;
    std::vector<TConsideration<FSeekWeaponContext>> Considerations;

    UASeekWeaponAction()
    {
        Name = "SeekWeapon";

        Considerations.push_back({
            "Unarmed",
            [](const FSeekWeaponContext& c){ return c.bHasWeapon ? 0.f : 1.f; },
            ECurveType::Linear
        });
        Considerations.push_back({
            "WeaponVisible",
            [](const FSeekWeaponContext& c){ return c.bWeaponVisible ? 1.f : 0.f; },
            ECurveType::Linear
        });
    }

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn&) override {}
    void OnExit (ASurvivorPawn&) override {}
    void Execute(ASurvivorPawn&, float)   override {}

    ISteeringBehavior* GetActiveBehavior() override { return &m_Arrive; }
    void SetTarget(const FTargetData& T) { m_Arrive.SetTarget(T); }

private:
    Arrive m_Arrive;
};

// ===========================================================================
// SCAVENGE ACTION
// Default behavior — visit houses one by one to look for items
// Scores high when no zombie is nearby (inverse proximity)
// ===========================================================================
struct FScavengeContext
{
    float NormalizedProximityToZombie = 0.f;
};

class UAScavengeAction : public IUtilityAction
{
public:
    FScavengeContext                              Context;
    std::vector<TConsideration<FScavengeContext>> Considerations;

    UAScavengeAction()
    {
        Name = "Scavenge";

        Considerations.push_back({
            "NoZombieNearby",
            [](const FScavengeContext& c){ return 1.f - c.NormalizedProximityToZombie; },
            ECurveType::Linear
        });
    }

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn& Agent) override
    {
        CollectHouses(Agent);
        PickNextHouse(Agent);
    }

    void OnExit(ASurvivorPawn&) override {}

    void Execute(ASurvivorPawn& Agent, float DeltaTime) override
    {
        // Bootstrap on very first tick if OnEnter was never called
        if (AllHouses.IsEmpty())
        {
            CollectHouses(Agent);
            PickNextHouse(Agent);
            return;
        }

        if (!CurrentTargetHouse)
        {
            PickNextHouse(Agent);
            return;
        }

        const float DistSq = FVector::DistSquared(
            Agent.GetActorLocation(), CurrentTargetHouse->GetActorLocation());

        if (DistSq < ArrivalRadiusSq)
        {
            TryPickUpItems(Agent);
            VisitedHouses.Add(CurrentTargetHouse);
            CurrentTargetHouse = nullptr;
            PickNextHouse(Agent);
        }
    }
    
    void TryPickUpItems(ASurvivorPawn& Agent)
    {
        UInventoryComponent* Inv = Agent.GetComponentByClass<UInventoryComponent>();
        if (!Inv) return;

        // Use a wider pickup sweep radius at the house (house bounds are large)
        const float SweepRadius = 400.f;
        const float SweepRadiusSq = SweepRadius * SweepRadius;
        const FVector MyLoc = Agent.GetActorLocation();

        for (TActorIterator<ABaseItem> It(Agent.GetWorld()); It; ++It)
        {
            ABaseItem* Item = *It;
            if (!Item) continue;
            if (Item->GetItemType() == EItemType::Garbage) continue;
            if (FVector::DistSquared(MyLoc, Item->GetActorLocation()) > SweepRadiusSq) continue;

            // Find a free slot for this item
            for (int32 i = 0; i < Inv->GetInventoryCapacity(); ++i)
            {
                if (Inv->GetInventory()[i] == nullptr)
                {
                    Inv->GrabItem(i, Item);
                    break; // next item
                }
            }
        }
    }

    ISteeringBehavior* GetActiveBehavior() override { return &m_PathFollow; }

private:
    PathFollow      m_PathFollow;
    TArray<AHouse*> AllHouses;
    TSet<AHouse*>   VisitedHouses;
    AHouse*         CurrentTargetHouse = nullptr;
    float           ArrivalRadiusSq    = 350.f * 350.f;

    void CollectHouses(ASurvivorPawn& Agent)
    {
        AllHouses.Reset();
        VisitedHouses.Empty();
        for (TActorIterator<AHouse> It(Agent.GetWorld()); It; ++It)
            AllHouses.Add(*It);
    }

    void PickNextHouse(ASurvivorPawn& Agent)
    {
        // All houses visited — reset and loop
        if (VisitedHouses.Num() >= AllHouses.Num())
            VisitedHouses.Empty();

        // Find nearest unvisited house
        AHouse* Best     = nullptr;
        float   BestDist = FLT_MAX;
        const FVector MyLoc = Agent.GetActorLocation();

        for (AHouse* H : AllHouses)
        {
            if (VisitedHouses.Contains(H)) continue;
            float Dist = FVector::Dist(MyLoc, H->GetActorLocation());
            if (Dist < BestDist) { BestDist = Dist; Best = H; }
        }

        CurrentTargetHouse = Best;
        if (Best)
        {
            // Use pawn's CalculatePath (UE NavMesh A*) to get navmesh-aware waypoints
            TArray<FVector> Path = Agent.CalculatePath(Best->GetActorLocation());
            if (Path.Num() > 0)
                m_PathFollow.SetPath(Path);
        }
    }
};