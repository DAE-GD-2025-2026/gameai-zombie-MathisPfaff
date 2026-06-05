#include "SurvivorUtilityAIComponent.h"
#include "AIController.h"
#include "StudentPerceptor.h"
#include "GameAI_Zombie/Survivor/SurvivorPawn.h"
#include "GameAI_Zombie/Common/HealthComponent.h"
#include "GameAI_Zombie/Common/InventoryComponent.h"
#include "GameAI_Zombie/Items/BaseItem.h"
#include "GameAI_Zombie/Items/ItemType.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameAI_Zombie/Zombies/BaseZombie.h"

USurvivorUtilityAIComponent::USurvivorUtilityAIComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorUtilityAIComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AAIController* Controller = Cast<AAIController>(GetOwner()))
        SurvivorPawn = Cast<ASurvivorPawn>(Controller->GetPawn());

    EvadeAction      = Brain.AddAction(std::make_unique<UAEvadeZombieAction>());
    ScavengeAction   = Brain.AddAction(std::make_unique<UAScavengeAction>());
    SeekMedkitAction = Brain.AddAction(std::make_unique<UASeekMedkitAction>());
    SeekWeaponAction = Brain.AddAction(std::make_unique<UASeekWeaponAction>());
    SeekFoodAction   = Brain.AddAction(std::make_unique<UASeekFoodAction>());
}

void USurvivorUtilityAIComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!SurvivorPawn) return;
    
    const FVector MyLoc = SurvivorPawn->GetActorLocation();

    // --- Perceptor & health ---
    UStudentPerceptor* Perceptor = SurvivorPawn->GetComponentByClass<UStudentPerceptor>();
    const TArray<AActor*>& Perceived = Perceptor ? Perceptor->GetPerceivedActors() : TArray<AActor*>{};

    float HealthRatio = 1.f;
    if (UHealthComponent* HP = SurvivorPawn->GetComponentByClass<UHealthComponent>())
        HealthRatio = FMath::Clamp((float)HP->GetHealth() / (float)HP->GetMaxHealth(), 0.f, 1.f);

    // --- Evade context ---
    if (Perceptor && Perceptor->HasZombieMemory())
    {
        FVector ZombiePos = Perceptor->GetLastKnownZombiePosition();
        // Normalize against evade distance — only urgent when zombie is actually close
        float ZombieDist = FVector::Dist(MyLoc, ZombiePos);
        float Proximity  = FMath::Clamp(1.f - (ZombieDist / EvadeMaxDistance), 0.f, 1.f);
        EvadeAction->Context.NormalizedProximityToZombie = Proximity;
        EvadeAction->SetTarget(FTargetData{ FVector2D(ZombiePos.X, ZombiePos.Y) });
    }
    else
    {
        EvadeAction->Context.NormalizedProximityToZombie = 0.f;
    }

    // --- House bounds check ---
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

    // --- Inventory state ---
    UInventoryComponent* Inv = SurvivorPawn->GetComponentByClass<UInventoryComponent>();
    bool bHasPistol    = InvHasType(Inv, EItemType::Pistol);
    bool bHasShotgun   = InvHasType(Inv, EItemType::Shotgun);
    bool bHasMedkit    = InvHasType(Inv, EItemType::Medkit);
    bool bFoodSlotFull = InvGetFreeSlotForType(Inv, EItemType::Food) == -1;

    // --- Nearest items from perception ---
    ABaseItem* NearestMedkit     = nullptr;
    ABaseItem* NearestWeapon     = nullptr;
    ABaseItem* NearestFood       = nullptr;
    float      NearestMedkitDist = FLT_MAX;
    float      NearestWeaponDist = FLT_MAX;
    float      NearestFoodDist   = FLT_MAX;
    float      NearestZombieDist = FLT_MAX;

    for (AActor* Actor : Perceived)
    {
        if (!Actor) continue;
        float Dist = FVector::Dist(MyLoc, Actor->GetActorLocation());

        if (Cast<ABaseZombie>(Actor))
        {
            NearestZombieDist = FMath::Min(NearestZombieDist, Dist);
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
    
    // Medkit — use memory fallback if not currently visible
    float MedkitDist = NearestMedkit
        ? NormDist(NearestMedkitDist, NearestMedkit)
        : SeekMedkitAction->GetMemoryDist();
    
    SeekMedkitAction->Context.bItemInInventory = bHasMedkit;
    SeekMedkitAction->Context.NormalizedDist   = MedkitDist;
    if (NearestMedkit)
        SeekMedkitAction->SetTarget(
            FTargetData{ FVector2D(NearestMedkit->GetActorLocation().X, NearestMedkit->GetActorLocation().Y) },
            NearestMedkit);
    
    // Weapon
    float WeaponDist = NearestWeapon
        ? NormDist(NearestWeaponDist, NearestWeapon)
        : SeekWeaponAction->GetMemoryDist();
    
    EItemType DesiredWeapon = (!bHasPistol) ? EItemType::Pistol : EItemType::Shotgun;
    SeekWeaponAction->SetWeaponType(DesiredWeapon);
    SeekWeaponAction->Context.bItemInInventory = (DesiredWeapon == EItemType::Pistol ? bHasPistol : bHasShotgun);
    SeekWeaponAction->Context.NormalizedDist   = WeaponDist;
    if (NearestWeapon)
        SeekWeaponAction->SetTarget(
            FTargetData{ FVector2D(NearestWeapon->GetActorLocation().X, NearestWeapon->GetActorLocation().Y) },
            NearestWeapon);
    
    // Food
    float FoodDist = NearestFood
        ? NormDist(NearestFoodDist, NearestFood)
        : SeekFoodAction->GetMemoryDist();
    
    SeekFoodAction->Context.bItemInInventory = bFoodSlotFull;
    SeekFoodAction->Context.NormalizedDist   = FoodDist;
    if (NearestFood)
        SeekFoodAction->SetTarget(
            FTargetData{ FVector2D(NearestFood->GetActorLocation().X, NearestFood->GetActorLocation().Y) },
            NearestFood);
    
    // Scavenge suppressed whenever any seek action has an active memory
    bool bAnyItemVisible = (MedkitDist > 0.f || WeaponDist > 0.f || FoodDist > 0.f);
    ScavengeAction->Context.NormalizedProximityToZombie = EvadeAction->Context.NormalizedProximityToZombie;
    ScavengeAction->Context.bItemVisible = bAnyItemVisible;

    // --- Run brain ---
    Brain.Update(*SurvivorPawn, DeltaTime);

    // --- Apply steering ---
    if (ISteeringBehavior* Active = Brain.GetActiveBehavior())
    {
        SteeringOutput Output = Active->CalculateSteering(DeltaTime, *SurvivorPawn);
        if (!Output.LinearVelocity.IsNearlyZero())
            SurvivorPawn->AddMovementInput(FVector(Output.LinearVelocity.X, Output.LinearVelocity.Y, 0), 1.f);
    }

    // Debug HUD
    GEngine->AddOnScreenDebugMessage(10, 0.f, FColor::Cyan,
        FString::Printf(TEXT("Action: %s | HP: %.0f%% | Pistol:%s Shotgun:%s Medkit:%s Food:%s"),
            *FString(Brain.GetCurrentActionName()),
            HealthRatio * 100.f,
            bHasPistol    ? TEXT("Y") : TEXT("N"),
            bHasShotgun   ? TEXT("Y") : TEXT("N"),
            bHasMedkit    ? TEXT("Y") : TEXT("N"),
            bFoodSlotFull ? TEXT("Full") : TEXT("N")));
}