#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_Carry.generated.h"

class AItemActor;

/**
 * 
 */
UCLASS()
class HEIST_API UGA_Carry : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Carry();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	// 에디터에서 GE_Carry 지정. MoveSpeed를 감소시키는 Infinite GE.
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Carry")
	TSubclassOf<UGameplayEffect> CarryEffect;

	FActiveGameplayEffectHandle CarryEffectHandle;

	AItemActor* Item;
};
