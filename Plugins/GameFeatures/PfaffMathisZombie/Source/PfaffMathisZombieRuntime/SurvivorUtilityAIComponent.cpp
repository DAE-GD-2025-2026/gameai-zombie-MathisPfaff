#include "SurvivorUtilityAIComponent.h"
#include "AIController.h"
#include "GameAI_Zombie/Survivor/SurvivorPawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "EngineUtils.h"
#include "GameAI_Zombie/Zombies/BaseZombie.h"

USurvivorUtilityAIComponent::USurvivorUtilityAIComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorUtilityAIComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AAIController* Controller = Cast<AAIController>(GetOwner()))
        SurvivorPawn = Cast<ASurvivorPawn>(Controller->GetPawn());

    // Register the evade action — Brain owns the memory
    EvadeAction = Brain.AddAction(std::make_unique<UAEvadeZombieAction>());
}

void USurvivorUtilityAIComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!SurvivorPawn) return;

    // --- Find nearest zombie and update context ---
    ABaseZombie* NearestZombie = nullptr;
    float NearestDist = FLT_MAX;

    for (TActorIterator<ABaseZombie> It(GetWorld()); It; ++It)
    {
        float Dist = FVector::Dist(SurvivorPawn->GetActorLocation(), It->GetActorLocation());
        if (Dist < NearestDist)
        {
            NearestDist = Dist;
            NearestZombie = *It;
        }
    }

    if (NearestZombie)
    {
        // Normalize: 1 = touching, 0 = at max distance or beyond
        EvadeAction->Context.NormalizedProximityToZombie =
            FMath::Clamp(1.f - (NearestDist / EvadeMaxDistance), 0.f, 1.f);

        FTargetData Target;
        Target.Position = FVector2D(NearestZombie->GetActorLocation());
        EvadeAction->SetTarget(Target);
    }
    else
    {
        EvadeAction->Context.NormalizedProximityToZombie = 0.f;
    }

    // --- Run utility brain ---
    Brain.Update(*SurvivorPawn, DeltaTime);

    // --- Apply steering output ---
    if (ISteeringBehavior* Active = Brain.GetActiveBehavior())
    {
        SteeringOutput Output = Active->CalculateSteering(DeltaTime, *SurvivorPawn);
        if (!Output.LinearVelocity.IsNearlyZero())
        {
            SurvivorPawn->AddMovementInput(FVector(Output.LinearVelocity.X, Output.LinearVelocity.Y, 0), 1.f);
        }
    }
}