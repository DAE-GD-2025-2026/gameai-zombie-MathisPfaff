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
}

void USurvivorUtilityAIComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!SurvivorPawn) return;

    // --- Update evade context using memory from perceptor ---
    UStudentPerceptor* Perceptor = SurvivorPawn->GetComponentByClass<UStudentPerceptor>();
    const TArray<AActor*>& Perceived = Perceptor ? Perceptor->GetPerceivedActors() : TArray<AActor*>{};

    // --- Health context ---
    float HealthRatio = 1.f;
    if (UHealthComponent* HP = SurvivorPawn->GetComponentByClass<UHealthComponent>())
        HealthRatio = FMath::Clamp((float)HP->GetHealth() / (float)HP->GetMaxHealth(), 0.f, 1.f);

    // --- Inventory context ---
    bool bHasWeapon = false;
    if (UInventoryComponent* Inv = SurvivorPawn->GetComponentByClass<UInventoryComponent>())
    {
        for (ABaseItem* Item : Inv->GetInventory())
        {
            if (Item && (Item->GetItemType() == EItemType::Pistol
                      || Item->GetItemType() == EItemType::Shotgun))
            {
                bHasWeapon = true;
                break;
            }
        }
    }

    // --- Scan perceived actors for zombies / medkits / weapons ---
    ABaseZombie* NearestZombie   = nullptr;
    ABaseItem*   NearestMedkit   = nullptr;
    ABaseItem*   NearestWeapon   = nullptr;
    float        NearestZombieDist  = FLT_MAX;
    float        NearestMedkitDist  = FLT_MAX;
    float        NearestWeaponDist  = FLT_MAX;

    const FVector MyLoc = SurvivorPawn->GetActorLocation();

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
            if (Item->GetItemType() == EItemType::Medkit && Dist < NearestMedkitDist)
                { NearestMedkitDist = Dist; NearestMedkit = Item; }
            else if ((Item->GetItemType() == EItemType::Pistol
                   || Item->GetItemType() == EItemType::Shotgun) && Dist < NearestWeaponDist)
                { NearestWeaponDist = Dist; NearestWeapon = Item; }
        }
    }

    // --- Update action contexts ---

    // Evade
    if (Perceptor && Perceptor->HasZombieMemory())
    {
        FVector ZombiePos = Perceptor->GetLastKnownZombiePosition();
        EvadeAction->Context.NormalizedProximityToZombie = 1.f;
        EvadeAction->SetTarget(FTargetData{ FVector2D(ZombiePos.X, ZombiePos.Y) });
    }
    else
    {
        EvadeAction->Context.NormalizedProximityToZombie = 0.f;
    }
    
    ScavengeAction->Context.NormalizedProximityToZombie =
        EvadeAction->Context.NormalizedProximityToZombie;

    // Medkit
    SeekMedkitAction->Context.NormalizedHealth = HealthRatio;
    SeekMedkitAction->Context.bMedkitVisible   = (NearestMedkit != nullptr);
    if (NearestMedkit)
        SeekMedkitAction->SetTarget(FTargetData{ FVector2D(NearestMedkit->GetActorLocation().X, NearestMedkit->GetActorLocation().Y) });
    // Weapon
    SeekWeaponAction->Context.bHasWeapon     = bHasWeapon;
    SeekWeaponAction->Context.bWeaponVisible = (NearestWeapon != nullptr);
    if (NearestWeapon)
        SeekWeaponAction->SetTarget(FTargetData{ FVector2D(NearestWeapon->GetActorLocation().X, NearestWeapon->GetActorLocation().Y) });
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
        FString::Printf(TEXT("Action: %s | HP: %.0f%% | Armed: %s"),
            *FString(Brain.GetCurrentActionName()),
            HealthRatio * 100.f,
            bHasWeapon ? TEXT("Yes") : TEXT("No")));
}