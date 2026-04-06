#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_PoliceEscort.generated.h"

class AThiefCharacter;

UCLASS()
class HEIST_API UGA_PoliceEscort : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PoliceEscort();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnArrivedAtCar(FGameplayEventData Payload);

private:
	UPROPERTY()
	TObjectPtr<AThiefCharacter> TargetThief;
};
