#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_Thief_StartEngine.generated.h"

/**
 * 
 */
UCLASS()
class HEIST_API UGA_Thief_StartEngine : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Thief_StartEngine();

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
};
