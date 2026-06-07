#include "StudentPerceptorPfaffMathis.h"
#include "GameFramework/Pawn.h"
#include "GameAI_Zombie/Zombies/BaseZombie.h"

UStudentPerceptorPfaffMathis::UStudentPerceptorPfaffMathis()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptorPfaffMathis::BeginPlay()
{
	Super::BeginPlay();

	if (auto* PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptorPfaffMathis::OnPerceptionUpdated);
}

void UStudentPerceptorPfaffMathis::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	PerceivedActors.RemoveAll([](AActor* A)
	{
		return !IsValid(A);
	});

	if (bScanning)
	{
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			FRotator Rot = OwnerPawn->GetActorRotation();
			Rot.Yaw += ScanRotationSpeed * DeltaTime;
			OwnerPawn->SetActorRotation(Rot);
		}
	}

	if (ZombieMemoryTimer > 0.f)
		ZombieMemoryTimer = FMath::Max(0.f, ZombieMemoryTimer - DeltaTime);

	for (AActor* Actor : PerceivedActors)
	{
		if (Actor && Actor->IsA<ABaseZombie>())
		{
			LastKnownZombiePosition = Actor->GetActorLocation();
			ZombieMemoryTimer = ZombieMemoryDuration;
		}
	}
}

void UStudentPerceptorPfaffMathis::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Stimulus.WasSuccessfullySensed())
	{
		PerceivedActors.AddUnique(Actor);
		GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green,
			FString::Printf(TEXT("Perceived: %s"), *Actor->GetName()));
	}
	else
	{
		PerceivedActors.Remove(Actor);
		if (Actor && Actor->IsA<ABaseZombie>())
			ZombieMemoryTimer = ZombieMemoryDuration;
	}
}