#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "StudentPerceptorPfaffMathis.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PFAFFMATHISZOMBIERUNTIME_API UStudentPerceptorPfaffMathis : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptorPfaffMathis();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY(EditDefaultsOnly, Category="Perception")
	float ScanRotationSpeed = 360.f;

	UPROPERTY(EditDefaultsOnly, Category="Perception")
	float ZombieMemoryDuration = 4.f;

	const TArray<AActor*>& GetPerceivedActors() const { return PerceivedActors; }

	bool HasZombieMemory() const { return ZombieMemoryTimer > 0.f; }
	FVector GetLastKnownZombiePosition() const { return LastKnownZombiePosition; }
	
	void StartScanning() { bScanning = true;  }
	void StopScanning()  { bScanning = false; }
	bool IsScanning()    const { return bScanning;}
	
private:
	TArray<AActor*> PerceivedActors;

	FVector LastKnownZombiePosition = FVector::ZeroVector;
	float   ZombieMemoryTimer       = 0.f;
	bool bScanning = true;
};