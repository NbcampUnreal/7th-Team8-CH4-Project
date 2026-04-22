#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_Thief_Depart.generated.h"

/**
 * 
 */
UCLASS()
class HEIST_API UGA_Thief_Depart : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Thief_Depart();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	// 채널링 오버라이드
	virtual void OnChannelingCompleted() override;
private:
	int32 GroupIndex = INDEX_NONE;
};
