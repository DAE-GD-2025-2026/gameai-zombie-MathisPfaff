#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkComponent.h"
#include "DecisionMaking/UtilityAIComponent.h"
#include "DecisionMaking/UtilityAction.h"
#include "SurvivorUtilityAIComponent.generated.h"

class ASurvivorPawn;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PFAFFMATHISZOMBIERUNTIME_API USurvivorUtilityAIComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	USurvivorUtilityAIComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	ASurvivorPawn* SurvivorPawn = nullptr;
	UtilityAIComponent Brain;
	UAEvadeZombieAction* EvadeAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="AI")
	float EvadeMaxDistance = 1500.f;
};