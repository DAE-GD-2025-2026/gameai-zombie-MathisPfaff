#pragma once

#include "MovementPfaffMathis/SteeringBehaviors/Steering/SteeringBehaviorsPfaffMathis.h"
#include <functional>
#include <vector>
#include <string>
#include "StudentPerceptorPfaffMathis.h"
#include "GameAI_Zombie/Items/BaseItem.h"
#include "GameAI_Zombie/Items/ItemType.h"
#include "GameAI_Zombie/Common/HealthComponent.h"
#include "GameAI_Zombie/Common/StaminaComponent.h"
#include "GameAI_Zombie/Items/Food.h"
#include "GameAI_Zombie/Common/InventoryComponent.h"
#include "EngineUtils.h"
#include "GameAI_Zombie/Village/House/House.h"
#include "MovementPfaffMathis/SteeringBehaviors/PathFollow/PathFollowSteeringBehaviorPfaffMathis.h"

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
class IUtilityActionPfaffMathis
{
public:
    std::string Name;
    virtual ~IUtilityActionPfaffMathis() = default;

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
// ===========================================================================
struct FEvadeZombieContext
{
    float NormalizedProximityToZombie = 0.f;
    bool bHasWeapon = false;
};

class UAEvadeZombieAction : public IUtilityActionPfaffMathis
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
        Considerations.push_back({
            "Unarmed",
            [](const FEvadeZombieContext& c){ return c.bHasWeapon ? 0.3f : 1.f; },
            ECurveType::Linear
        });
    }

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn& Agent) override
    {
        if (auto* P = Agent.GetComponentByClass<UStudentPerceptorPfaffMathis>()) P->StartScanning();
    
        if (UStaminaComponent* ST = Agent.GetComponentByClass<UStaminaComponent>())
            if (ST->GetCurrentStamina() > 0.f)
                Agent.StartRunning();
    }

    void OnExit(ASurvivorPawn& Agent) override
    {
        Agent.StopRunning();
    }

    void Execute(ASurvivorPawn& Agent, float DeltaTime) override
    {
        UStaminaComponent* ST  = Agent.GetComponentByClass<UStaminaComponent>();
        UInventoryComponent* Inv = Agent.GetComponentByClass<UInventoryComponent>();
        UHealthComponent* HP   = Agent.GetComponentByClass<UHealthComponent>();

        if (ST)
        {
            if (Agent.IsRunning() && ST->GetCurrentStamina() <= 0.f)
            {
                Agent.StopRunning();
                if (Inv) { if (!Inv->UseItem(3)) Inv->UseItem(4); }
            }
            else if (!Agent.IsRunning() && ST->GetCurrentStamina() > 0.f)
            {
                Agent.StartRunning();
            }
        }

        if (HP && Inv)
        {
            float HealthRatio = FMath::Clamp((float)HP->GetHealth() / (float)HP->GetMaxHealth(), 0.f, 1.f);
            if (HealthRatio < 0.4f)
                Inv->UseItem(2);
        }
    }

    void SetZombieTargets(const TArray<FTargetData>& Targets)
    {
        ZombieTargets = Targets;
        while (m_Evades.size() < static_cast<size_t>(ZombieTargets.Num()))
            m_Evades.emplace_back();
        for (int32 i = 0; i < ZombieTargets.Num(); ++i)
            m_Evades[i].SetTarget(ZombieTargets[i]);
        m_MultiEvade.SetZombies(&m_Evades, &ZombieTargets);
    }

    void SetTarget(const FTargetData& T)
    {
        TArray<FTargetData> S; S.Add(T);
        SetZombieTargets(S);
    }

    ISteeringBehavior* GetActiveBehavior() override { return &m_MultiEvade; }

private:
    struct FMultiEvadeSteering : public ISteeringBehavior
    {
        std::vector<Evade>*   Evades  = nullptr;
        TArray<FTargetData>*  Targets = nullptr;

        void SetZombies(std::vector<Evade>* E, TArray<FTargetData>* T)
        {
            Evades = E; Targets = T;
        }

        SteeringOutput CalculateSteering(float DeltaT, ASurvivorPawn& Agent) override
        {
            SteeringOutput Output{};
            if (!Evades || !Targets || Targets->IsEmpty()) return Output;

            const FVector2D AgentPos  = FVector2D(Agent.GetActorLocation());
            FVector2D BlendedDir      = FVector2D::ZeroVector;
            float     TotalWeight     = 0.f;

            for (int32 i = 0; i < Targets->Num(); ++i)
            {
                SteeringOutput EvadeOut = (*Evades)[i].CalculateSteering(DeltaT, Agent);
                if (EvadeOut.LinearVelocity.IsNearlyZero()) continue;

                float Dist   = FVector2D::Distance(AgentPos, (*Targets)[i].Position);
                float Weight = (Dist > 1.f) ? (1.f / Dist) : 1.f;

                FVector2D Dir = EvadeOut.LinearVelocity;
                Dir.Normalize();
                BlendedDir  += Dir * Weight;
                TotalWeight += Weight;
            }

            if (TotalWeight > 0.f)
                BlendedDir /= TotalWeight;

            if (BlendedDir.SizeSquared() < 0.01f && Targets->Num() > 0)
            {
                FVector2D Fallback = AgentPos - (*Targets)[0].Position;
                if (!Fallback.IsNearlyZero())
                {
                    Fallback.Normalize();
                    BlendedDir = FVector2D(-Fallback.Y, Fallback.X);
                }
            }

            Output.LinearVelocity = BlendedDir;
            return Output;
        }
    };

    std::vector<Evade>    m_Evades;
    TArray<FTargetData>   ZombieTargets;
    FMultiEvadeSteering   m_MultiEvade;
};

// ===========================================================================
// FIGHT ZOMBIE ACTION
// ===========================================================================
struct FFightZombieContext
{
    float NormalizedProximityToZombie = 0.f;
    bool  bHasWeapon                  = false;
    float HealthRatio                 = 1.f;
};

class UAFightZombieAction : public IUtilityActionPfaffMathis
{
public:
    FFightZombieContext                              Context;
    std::vector<TConsideration<FFightZombieContext>> Considerations;

    UAFightZombieAction()
    {
        Name = "FightZombie";

        Considerations.push_back({
            "ZombieClose",
            [](const FFightZombieContext& c){ return c.NormalizedProximityToZombie; },
            ECurveType::Exponential
        });
        Considerations.push_back({
            "HasWeapon",
            [](const FFightZombieContext& c){ return c.bHasWeapon ? 1.f : 0.f; },
            ECurveType::Linear
        });
        Considerations.push_back({
            "HealthNotCritical",
            [](const FFightZombieContext& c){ return c.HealthRatio; },
            ECurveType::Linear
        });
    }

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn& Agent) override
    {
        FireCooldown = 0.f;
        if (auto* P = Agent.GetComponentByClass<UStudentPerceptorPfaffMathis>()) P->StopScanning();
    }
    void OnExit(ASurvivorPawn& Agent) override {}

    void Execute(ASurvivorPawn& Agent, float DeltaTime) override
    {
        FireCooldown -= DeltaTime;
        if (FireCooldown > 0.f) return;

        UInventoryComponent* Inv = Agent.GetComponentByClass<UInventoryComponent>();
        if (!Inv) return;

        if (Inv->UseItem(1))
            FireCooldown = ShotgunCooldown;
        else if (Inv->UseItem(0))
            FireCooldown = PistolCooldown;
    }

    ISteeringBehavior* GetActiveBehavior() override { return &m_Face; }

    void SetZombieTarget(const FTargetData& T) { m_Face.SetTarget(T); }

private:
    Face  m_Face;
    float FireCooldown    = 0.f;
    float PistolCooldown  = 0.1f;  
    float ShotgunCooldown = 0.1f;
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
// Base class shared by Medkit / Weapon / Food actions
// ===========================================================================
struct FSeekItemContext
{
    bool  bItemInInventory = false;
    float NormalizedDist   = 0.f;
};

class UASeekItemActionBase : public IUtilityActionPfaffMathis
{
public:
    FSeekItemContext                              Context;
    std::vector<TConsideration<FSeekItemContext>> Considerations;

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn& Agent) override
    {
        if (auto* P = Agent.GetComponentByClass<UStudentPerceptorPfaffMathis>()) P->StopScanning();
    }
    void OnExit(ASurvivorPawn& Agent) override
    {
        MemoryTimer = 0.f;
    }
    void Execute(ASurvivorPawn& Agent, float DeltaTime) override
    {
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

    void SetTarget(const FTargetData& T, ABaseItem* Item)
    {
        m_Arrive.SetTarget(T);
        TargetItem  = Item;
        MemoryTimer = MemoryDuration;
    }

    float GetMemoryDist() const { return MemoryTimer > 0.f ? 1.f : 0.f; }

protected:
    EItemType  ItemTypeToGrab  = EItemType::Garbage;
    ABaseItem* TargetItem      = nullptr;
    float      GrabRadiusSq    = 100.f * 100.f;
    float      MemoryTimer     = 0.f;
    float      MemoryDuration  = 5.f;

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
// SEEK WEAPON ACTION
// ===========================================================================
class UASeekWeaponAction : public UASeekItemActionBase
{
public:
    UASeekWeaponAction()
    {
        Name           = "SeekWeapon";
        ItemTypeToGrab = EItemType::Pistol;

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
// ===========================================================================
struct FScavengeContext
{
    float NormalizedProximityToZombie = 0.f;
    bool  bItemVisible                = false;
};

class UAScavengeAction : public IUtilityActionPfaffMathis
{
public:
    FScavengeContext                              Context;
    std::vector<TConsideration<FScavengeContext>> Considerations;
    
    void UpdatePerceivedHouses(const TArray<AActor*>& Perceived)
    {
        for (AActor* Actor : Perceived)
        {
            if (AHouse* H = Cast<AHouse>(Actor))
                KnownHouses.AddUnique(H);
        }
        KnownHouses.RemoveAll([](AHouse* H){ return !IsValid(H); });
    }

    UAScavengeAction()
    {
        Name = "Scavenge";

        Considerations.push_back({
            "NoZombieNearby",
            [](const FScavengeContext& c){ return FMath::Max(0.05f, 1.f - c.NormalizedProximityToZombie); },
            ECurveType::Linear
        });
        Considerations.push_back({
            "NoItemVisible",
            [](const FScavengeContext& c){ return c.bItemVisible ? 0.05f : 1.f; },
            ECurveType::Linear
        });
    }

    float Score() const override { return CalcScore(Considerations, Context); }

    void OnEnter(ASurvivorPawn& Agent) override
    {
        if (auto* P = Agent.GetComponentByClass<UStudentPerceptorPfaffMathis>()) P->StartScanning();
        PickNextTarget(Agent);
    }
    
    void OnExit(ASurvivorPawn& Agent) override
    {
        if (auto* P = Agent.GetComponentByClass<UStudentPerceptorPfaffMathis>()) P->StopScanning();
    }

    void Execute(ASurvivorPawn& Agent, float DeltaTime) override
    {
        for (auto& Pair : HouseCooldowns)
            Pair.Value = FMath::Max(0.f, Pair.Value - DeltaTime);

        bool bShouldWander = false;

        if (KnownHouses.IsEmpty())
        {
            bShouldWander = true;
        }
        else
        {
            if (!CurrentTargetHouse)
                PickNextTarget(Agent);

            if (CurrentTargetHouse)
            {
                const float DistSq = FVector::DistSquared(
                    Agent.GetActorLocation(), CurrentTargetHouse->GetActorLocation());

                if (DistSq < ArrivalRadiusSq)
                {
                    HouseCooldowns.Add(CurrentTargetHouse, HouseRevisitCooldown);
                    CurrentTargetHouse = nullptr;
                    PickNextTarget(Agent);
                }
            }

            bShouldWander = (CurrentTargetHouse == nullptr);
        }

        if (bShouldWander != bUseWander)
        {
            bUseWander = bShouldWander;
            if (auto* P = Agent.GetComponentByClass<UStudentPerceptorPfaffMathis>())
            {
                if (bUseWander)
                    P->StopScanning();
                else
                    P->StartScanning();
            }
        }
        else
        {
            bUseWander = bShouldWander;
        }
    }

    ISteeringBehavior* GetActiveBehavior() override
    {
        return bUseWander ? static_cast<ISteeringBehavior*>(&m_Wander)
                          : static_cast<ISteeringBehavior*>(&m_PathFollow);
    }
    
private:
    PathFollow      m_PathFollow;
    Wander          m_Wander;
    TArray<AHouse*> KnownHouses;
    TMap<AHouse*, float> HouseCooldowns;
    float HouseRevisitCooldown = 30.f;
    AHouse*         CurrentTargetHouse = nullptr;
    float           ArrivalRadiusSq    = 350.f * 350.f;
    bool            bUseWander         = false;

    void PickNextTarget(ASurvivorPawn& Agent)
    {
        if (KnownHouses.IsEmpty()) { bUseWander = true; return; }

        AHouse* Best     = nullptr;
        float   BestDist = FLT_MAX;
        const FVector MyLoc = Agent.GetActorLocation();

        for (AHouse* H : KnownHouses)
        {
            float* Cooldown = HouseCooldowns.Find(H);
            if (Cooldown && *Cooldown > 0.f) continue;

            float Dist = FVector::Dist(MyLoc, H->GetActorLocation());
            if (Dist < BestDist) { BestDist = Dist; Best = H; }
        }

        CurrentTargetHouse = Best;
        bUseWander = (Best == nullptr);

        if (Best)
        {
            TArray<FVector> Path = Agent.CalculatePath(Best->GetActorLocation());
            if (Path.Num() > 0)
                m_PathFollow.SetPath(Path);
        }
    }
};