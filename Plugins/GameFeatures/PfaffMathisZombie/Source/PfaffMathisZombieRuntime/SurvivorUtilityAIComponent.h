#pragma once

#include "CoreMinimal.h"
#include "DecisionMaking/UtilityAIComponent.h"
#include "DecisionMaking/UtilityAction.h"
#include "Components/ActorComponent.h"
#include "SurvivorUtilityAIComponent.generated.h"

class ASurvivorPawn;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PFAFFMATHISZOMBIERUNTIME_API USurvivorUtilityAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivorUtilityAIComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	ASurvivorPawn*         SurvivorPawn    = nullptr;
	UtilityAIComponent     Brain;

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