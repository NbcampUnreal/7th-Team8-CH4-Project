#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_Thief_Heal.generated.h"

class UAudioComponent;

UCLASS()
class HEIST_API UGA_Thief_Heal : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Thief_Heal();

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

protected:

	// 타겟 ASC 캐싱
	UPROPERTY()
	TWeakObjectPtr<class UAbilitySystemComponent> TargetASC;

	UPROPERTY()
	TObjectPtr<UAudioComponent> HealAudioComp;
};
