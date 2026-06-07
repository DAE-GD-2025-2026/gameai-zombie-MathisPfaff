#include "SurvivorUtilityAIComponentPfaffMathis.h"
#include "AIController.h"
#include "StudentPerceptorPfaffMathis.h"
#include "GameAI_Zombie/Survivor/SurvivorPawn.h"
#include "GameAI_Zombie/Common/HealthComponent.h"
#include "GameAI_Zombie/Common/InventoryComponent.h"
#include "GameAI_Zombie/Items/BaseItem.h"
#include "GameAI_Zombie/Items/ItemType.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameAI_Zombie/Zombies/BaseZombie.h"

USurvivorUtilityAIComponentPfaffMathis::USurvivorUtilityAIComponentPfaffMathis()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorUtilityAIComponentPfaffMathis::BeginPlay()
{
    Super::BeginPlay();

    if (AAIController* Controller = Cast<AAIController>(GetOwner()))
        SurvivorPawn = Cast<ASurvivorPawn>(Controller->GetPawn());

    EvadeAction      = Brain.AddAction(std::make_unique<UAEvadeZombieAction>());
    ScavengeAction   = Brain.AddAction(std::make_unique<UAScavengeAction>());
    SeekMedkitAction = Brain.AddAction(std::make_unique<UASeekMedkitAction>());
    SeekWeaponAction = Brain.AddAction(std::make_unique<UASeekWeaponAction>());
    SeekFoodAction   = Brain.AddAction(std::make_unique<UASeekFoodAction>());
    FightAction      = Brain.AddAction(std::make_unique<UAFightZombieAction>());
}

void USurvivorUtilityAIComponentPfaffMathis::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!SurvivorPawn) return;
    
    const FVector MyLoc = SurvivorPawn->GetActorLocation();

    UStudentPerceptorPfaffMathis* Perceptor = SurvivorPawn->GetComponentByClass<UStudentPerceptorPfaffMathis>();
    const TArray<AActor*>& Perceived = Perceptor ? Perceptor->GetPerceivedActors() : TArray<AActor*>{};

    float HealthRatio = 1.f;
    if (UHealthComponent* HP = SurvivorPawn->GetComponentByClass<UHealthComponent>())
        HealthRatio = FMath::Clamp((float)HP->GetHealth() / (float)HP->GetMaxHealth(), 0.f, 1.f);

    UInventoryComponent* Inv = SurvivorPawn->GetComponentByClass<UInventoryComponent>();
    bool bHasPistol    = InvHasType(Inv, EItemType::Pistol);
    bool bHasShotgun   = InvHasType(Inv, EItemType::Shotgun);
    bool bHasMedkit    = InvHasType(Inv, EItemType::Medkit);
    bool bFoodSlotFull = InvGetFreeSlotForType(Inv, EItemType::Food) == -1;
    
    bool bHasWeapon = bHasPistol || bHasShotgun;
    
    TArray<FTargetData> ZombieTargetData;
    float ClosestZombieDist = FLT_MAX;

    if (Perceptor)
    {
        for (AActor* Actor : Perceived)
        {
            if (!Actor) continue;
            if (Cast<ABaseZombie>(Actor))
            {
                float Dist = FVector::Dist(MyLoc, Actor->GetActorLocation());
                ClosestZombieDist = FMath::Min(ClosestZombieDist, Dist);

                FTargetData TD;
                TD.Position        = FVector2D(Actor->GetActorLocation());
                TD.LinearVelocity  = FVector2D(Actor->GetVelocity());
                ZombieTargetData.Add(TD);
            }
        }
        
        ScavengeAction->UpdatePerceivedHouses(Perceived);

        if (ZombieTargetData.IsEmpty() && Perceptor->HasZombieMemory())
        {
            FVector ZombiePos = Perceptor->GetLastKnownZombiePosition();
            float MemDist = FVector::Dist(MyLoc, ZombiePos);

            if (MemDist < EvadeMaxDistance)
            {
                ClosestZombieDist = MemDist;
                FTargetData TD;
                TD.Position = FVector2D(ZombiePos);
                ZombieTargetData.Add(TD);
            }
        }
    }

    if (!ZombieTargetData.IsEmpty())
    {
        float Proximity = FMath::Clamp(1.f - (ClosestZombieDist / EvadeMaxDistance), 0.f, 1.f);
        EvadeAction->Context.NormalizedProximityToZombie = Proximity;
        EvadeAction->Context.bHasWeapon = bHasWeapon;
        EvadeAction->SetZombieTargets(ZombieTargetData);
    }
    else
    {
        EvadeAction->Context.NormalizedProximityToZombie = 0.f;
    }

    bool bSurvivorInHouse = false;
    for (TActorIterator<AHouse> It(SurvivorPawn->GetWorld()); It; ++It)
    {
        FHouseBounds B = (*It)->GetBounds();
        if (FMath::Abs(MyLoc.X - B.Origin.X) <= B.Extent.X &&
            FMath::Abs(MyLoc.Y - B.Origin.Y) <= B.Extent.Y)
        {
            bSurvivorInHouse = true;
            break;
        }
    }
    
    ABaseItem* NearestMedkit     = nullptr;
    ABaseItem* NearestWeapon     = nullptr;
    ABaseItem* NearestFood       = nullptr;
    ABaseZombie* NearestZombie   = nullptr;
    float      NearestMedkitDist = FLT_MAX;
    float      NearestWeaponDist = FLT_MAX;
    float      NearestFoodDist   = FLT_MAX;
    float      NearestZombieDist = FLT_MAX;
    
    EItemType DesiredWeapon = (!bHasPistol) ? EItemType::Pistol : EItemType::Shotgun;

    for (AActor* Actor : Perceived)
    {
        if (!Actor) continue;
        float Dist = FVector::Dist(MyLoc, Actor->GetActorLocation());

        if (ABaseZombie* Z = Cast<ABaseZombie>(Actor))
        {
            if (Dist < NearestZombieDist) { NearestZombieDist = Dist; NearestZombie = Z; }
        }
        else if (ABaseItem* Item = Cast<ABaseItem>(Actor))
        {
            switch (Item->GetItemType())
            {
                case EItemType::Medkit:
                    if (Dist < NearestMedkitDist) { NearestMedkitDist = Dist; NearestMedkit = Item; }
                    break;
                case EItemType::Pistol:
                case EItemType::Shotgun:
                    if (Item->GetItemType() == DesiredWeapon)
                        if (Dist < NearestWeaponDist) { NearestWeaponDist = Dist; NearestWeapon = Item; }
                    break;
                case EItemType::Food:
                    if (Dist < NearestFoodDist)   { NearestFoodDist = Dist;   NearestFood = Item; }
                    break;
                default: break;
            }
        }
    }


    const float MaxDist = 1000.f;
    auto NormDist = [&](float Dist, ABaseItem* Item) -> float {
        if (!Item) return 0.f;
        const float CloseBonus = FMath::Clamp(1.f - (Dist / MaxDist), 0.f, 1.f);
        return 0.5f + CloseBonus * 0.5f;
    };
    
    float MedkitDist = NearestMedkit
        ? NormDist(NearestMedkitDist, NearestMedkit)
        : SeekMedkitAction->GetMemoryDist();
    
    SeekMedkitAction->Context.bItemInInventory = bHasMedkit;
    SeekMedkitAction->Context.NormalizedDist   = MedkitDist;
    if (NearestMedkit)
        SeekMedkitAction->SetTarget(
            FTargetData{ FVector2D(NearestMedkit->GetActorLocation().X, NearestMedkit->GetActorLocation().Y) },
            NearestMedkit);
    
    float WeaponDist = NearestWeapon
        ? NormDist(NearestWeaponDist, NearestWeapon)
        : SeekWeaponAction->GetMemoryDist();
    
    SeekWeaponAction->SetWeaponType(DesiredWeapon);
    SeekWeaponAction->Context.bItemInInventory = (DesiredWeapon == EItemType::Pistol ? bHasPistol : bHasShotgun);
    SeekWeaponAction->Context.NormalizedDist   = WeaponDist;
    if (NearestWeapon)
        SeekWeaponAction->SetTarget(
            FTargetData{ FVector2D(NearestWeapon->GetActorLocation().X, NearestWeapon->GetActorLocation().Y) },
            NearestWeapon);
    
    float FoodDist = NearestFood
        ? NormDist(NearestFoodDist, NearestFood)
        : SeekFoodAction->GetMemoryDist();
    
    SeekFoodAction->Context.bItemInInventory = bFoodSlotFull;
    SeekFoodAction->Context.NormalizedDist   = FoodDist;
    if (NearestFood)
        SeekFoodAction->SetTarget(
            FTargetData{ FVector2D(NearestFood->GetActorLocation().X, NearestFood->GetActorLocation().Y) },
            NearestFood);
    
    if (NearestZombie)
    {
        FVector ZPos = NearestZombie->GetActorLocation();
        float Proximity = FMath::Clamp(1.f - (NearestZombieDist / FightMaxDistance), 0.f, 1.f);
        FightAction->Context.NormalizedProximityToZombie = Proximity;
        FightAction->Context.bHasWeapon  = bHasWeapon;
        FightAction->Context.HealthRatio = HealthRatio;
        FightAction->SetZombieTarget(FTargetData{ FVector2D(ZPos) });
    }
    else
    {
        FightAction->Context.NormalizedProximityToZombie = 0.f;
        FightAction->Context.bHasWeapon  = false;
    }
    
    bool bAnyItemVisible = (NearestMedkit != nullptr || NearestWeapon != nullptr || NearestFood != nullptr);    ScavengeAction->Context.NormalizedProximityToZombie = EvadeAction->Context.NormalizedProximityToZombie;
    ScavengeAction->Context.bItemVisible = bAnyItemVisible;

    Brain.Update(*SurvivorPawn, DeltaTime);

    if (ISteeringBehavior* Active = Brain.GetActiveBehavior())
    {
        SteeringOutput Output = Active->CalculateSteering(DeltaTime, *SurvivorPawn);

        if (!Output.LinearVelocity.IsNearlyZero())
            SurvivorPawn->AddMovementInput(FVector(Output.LinearVelocity.X, Output.LinearVelocity.Y, 0), 1.f);

        if (FMath::Abs(Output.AngularVelocity) > KINDA_SMALL_NUMBER)
        {
            FRotator Rot = SurvivorPawn->GetActorRotation();
            Rot.Yaw += Output.AngularVelocity * 180.f * DeltaTime;
            SurvivorPawn->SetActorRotation(Rot);
        }
    }

    GEngine->AddOnScreenDebugMessage(10, 0.f, FColor::Cyan,
        FString::Printf(TEXT("Action: %s | HP: %.0f%% | Pistol:%s Shotgun:%s Medkit:%s Food:%s"),
            *FString(Brain.GetCurrentActionName()),
            HealthRatio * 100.f,
            bHasPistol    ? TEXT("Y") : TEXT("N"),
            bHasShotgun   ? TEXT("Y") : TEXT("N"),
            bHasMedkit    ? TEXT("Y") : TEXT("N"),
            bFoodSlotFull ? TEXT("Full") : TEXT("N")));
}