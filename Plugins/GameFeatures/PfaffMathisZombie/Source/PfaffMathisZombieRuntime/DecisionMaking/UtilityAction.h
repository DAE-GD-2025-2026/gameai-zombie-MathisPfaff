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

inline bool InvHasType(UInventoryComponent* Inv, EItemType Type)
{
    if (!Inv) return false;
    for (ABaseItem* Item : Inv->GetInventory())
        if (Item && Item->GetItemType() == Type)
            return true;
    return false;
}

inline int InvGetFreeSlotForType(UInventoryComponent* Inv, EItemType Type)
{
    if (!Inv) return -1;
    const TArray<ABaseItem*>& Items = Inv->GetInventory();
    TArray<int> Slots;
    switch (Type)
    {
    case EItemType::Pistol:  Slots = {0};    break;
    case EItemType::Shotgun: Slots = {1};    break;
    case EItemType::Medkit:  Slots = {2};    break;
    case EItemType::Food:    Slots = {3, 4}; break;
    default: return -1;
    }
    for (int Idx : Slots)
        if (Idx < Items.Num() && Items[Idx] == nullptr)
            return Idx;
    return -1;
}

// ===========================================================================
// Shared context for "seek a specific item" actions
// bIsInHouse        — survivor is inside a house (boosts score slightly)
// bItemInInventory  — already carrying one (kills score → action won't fire)
// NormalizedDist    — 0=far/not seen, 1=right on top of it
// ===========================================================================
struct FSeekItemContext
{
    bool  bItemInInventory = false;
    float NormalizedDist   = 0.f;   // 1 = at pickup range, 0 = not visible / far away
};

// ---------------------------------------------------------------------------
// Base class shared by Medkit / Weapon / Food actions
// ---------------------------------------------------------------------------
class UASeekItemActionBase : public IUtilityAction
{
public:
    FSeekItemContext                              Context;
    std::vector<TConsideration<FSeekItemContext>> Considerations;

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn&) override {}
    void OnExit (ASurvivorPawn&) override { MemoryTimer = 0.f; }  // clear on exit

    void Execute(ASurvivorPawn& Agent, float DeltaTime) override
    {
        // Count down memory
        if (MemoryTimer > 0.f)
            MemoryTimer = FMath::Max(0.f, MemoryTimer - DeltaTime);

        if (!TargetItem) return;

        UInventoryComponent* Inv = Agent.GetComponentByClass<UInventoryComponent>();
        if (!Inv) return;

        const float DistSq = FVector::DistSquared(
            Agent.GetActorLocation(), TargetItem->GetActorLocation());

        if (DistSq <= GrabRadiusSq)
        {
            int Slot = InvGetFreeSlotForType(Inv, ItemTypeToGrab);
            if (Slot != -1)
                Inv->GrabItem(Slot, TargetItem);
            TargetItem  = nullptr;
            MemoryTimer = 0.f;
        }
    }

    ISteeringBehavior* GetActiveBehavior() override { return &m_Arrive; }

    // Called from TickComponent when item IS visible
    void SetTarget(const FTargetData& T, ABaseItem* Item)
    {
        m_Arrive.SetTarget(T);
        TargetItem  = Item;
        MemoryTimer = MemoryDuration; // refresh memory
    }

    // Call every tick so NormalizedDist stays high while item is remembered
    float GetMemoryDist() const { return MemoryTimer > 0.f ? 1.f : 0.f; }

protected:
    EItemType  ItemTypeToGrab  = EItemType::Garbage;
    ABaseItem* TargetItem      = nullptr;
    float      GrabRadiusSq    = 100.f * 100.f;
    float      MemoryTimer     = 0.f;
    float      MemoryDuration  = 5.f;   // seconds to keep targeting after losing sight

    Arrive m_Arrive;
};

// ===========================================================================
// SEEK MEDKIT ACTION
// ===========================================================================
class UASeekMedkitAction : public UASeekItemActionBase
{
public:
    UASeekMedkitAction()
    {
        Name            = "SeekMedkit";
        ItemTypeToGrab  = EItemType::Medkit;

        // Fires hard when health is low AND medkit visible AND not already carried
        Considerations.push_back({
            "NoMedkitInInventory",
            [](const FSeekItemContext& c){ return c.bItemInInventory ? 0.f : 1.f; },
            ECurveType::Linear
        });
        Considerations.push_back({
            "MedkitClose",
            [](const FSeekItemContext& c){ return c.NormalizedDist; },
            ECurveType::Linear
        });
    }
};

// ===========================================================================
// SEEK WEAPON ACTION  (pistol + shotgun share one action; it prefers pistol
//                      first, then shotgun if pistol slot is already filled)
// ===========================================================================
class UASeekWeaponAction : public UASeekItemActionBase
{
public:
    UASeekWeaponAction()
    {
        Name           = "SeekWeapon";
        ItemTypeToGrab = EItemType::Pistol; // updated dynamically in context update

        Considerations.push_back({
            "NoWeaponInInventory",
            [](const FSeekItemContext& c){ return c.bItemInInventory ? 0.f : 1.f; },
            ECurveType::Linear
        });
        Considerations.push_back({
            "WeaponClose",
            [](const FSeekItemContext& c){ return c.NormalizedDist; },
            ECurveType::Linear
        });
    }

    // Call this from the context update so GrabItem uses the right slot
    void SetWeaponType(EItemType T) { ItemTypeToGrab = T; }
};

// ===========================================================================
// SEEK FOOD ACTION
// ===========================================================================
class UASeekFoodAction : public UASeekItemActionBase
{
public:
    UASeekFoodAction()
    {
        Name           = "SeekFood";
        ItemTypeToGrab = EItemType::Food;

        Considerations.push_back({
            "FoodSlotFree",
            [](const FSeekItemContext& c){ return c.bItemInInventory ? 0.f : 1.f; },
            ECurveType::Linear
        });
        Considerations.push_back({
            "FoodClose",
            [](const FSeekItemContext& c){ return c.NormalizedDist; },
            ECurveType::Linear
        });
    }
};

// ===========================================================================
// SCAVENGE ACTION
// Default behavior — visit houses one by one to look for items
// Scores high when no zombie is nearby (inverse proximity)
// ===========================================================================
struct FScavengeContext
{
    float NormalizedProximityToZombie = 0.f;
    bool  bItemVisible                = false;
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
        Considerations.push_back({
            "NoItemVisible",
            [](const FScavengeContext& c){ return c.bItemVisible ? 0.f : 1.f; },
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

    void Execute(ASurvivorPawn& Agent, float /*DeltaTime*/) override
    {
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
            VisitedHouses.Add(CurrentTargetHouse);
            CurrentTargetHouse = nullptr;
            PickNextHouse(Agent);
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