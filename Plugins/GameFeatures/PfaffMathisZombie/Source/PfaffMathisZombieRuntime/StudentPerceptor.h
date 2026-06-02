#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "StudentPerceptor.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PFAFFMATHISZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptor();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY(EditDefaultsOnly, Category="Perception")
	float ScanRotationSpeed = 360.f;

	// How long (seconds) the survivor keeps fleeing after losing sight of a zombie
	UPROPERTY(EditDefaultsOnly, Category="Perception")
	float ZombieMemoryDuration = 4.f;

	const TArray<AActor*>& GetPerceivedActors() const { return PerceivedActors; }

	// Returns true if a zombie was seen recently (either now or within memory window)
	bool HasZombieMemory() const { return ZombieMemoryTimer > 0.f; }
	FVector GetLastKnownZombiePosition() const { return LastKnownZombiePosition; }

private:
	TArray<AActor*> PerceivedActors;

	FVector LastKnownZombiePosition = FVector::ZeroVector;
	float   ZombieMemoryTimer       = 0.f; // counts down after zombie lost
};