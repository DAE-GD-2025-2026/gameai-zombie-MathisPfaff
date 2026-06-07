#pragma once

#include "CoreMinimal.h"
#include "DecisionMakingPfaffMathis/UtilityAIComponentPfaffMathis.h"
#include "DecisionMakingPfaffMathis/UtilityActionPfaffMathis.h"
#include "Components/ActorComponent.h"
#include "SurvivorUtilityAIComponentPfaffMathis.generated.h"

class ASurvivorPawn;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PFAFFMATHISZOMBIERUNTIME_API USurvivorUtilityAIComponentPfaffMathis : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivorUtilityAIComponentPfaffMathis();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	ASurvivorPawn*         SurvivorPawn    = nullptr;
	UtilityAIComponentPfaffMathis     Brain;

	UAEvadeZombieAction*   EvadeAction		= nullptr;
	UAScavengeAction*      ScavengeAction   = nullptr;
	UASeekMedkitAction*    SeekMedkitAction = nullptr;
	UASeekWeaponAction*    SeekWeaponAction = nullptr;
	UASeekFoodAction*      SeekFoodAction   = nullptr;
	UAFightZombieAction* FightAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="AI")
	float FightMaxDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category="AI")
	float EvadeMaxDistance = 1500.f;
};