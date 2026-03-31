#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_Sneak.generated.h"

/**
 * 홀드 중 슬금슬금 이동. 입력을 누르는 동안 State.Sneaking 태그를 유지한다.
 * ActivationPolicy = WhileInputActive
 *
 * 에디터에서 SneakEffect에 GE_Sneak을 지정한다.
 * GE_Sneak은 MoveSpeed 어트리뷰트에 Additive 감소를 적용한다.
 */
UCLASS()
class HEIST_API UGA_Sneak : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Sneak();

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
	// 에디터에서 GE_Sneak 지정. MoveSpeed를 감소시키는 Infinite GE.
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Sneak")
	TSubclassOf<UGameplayEffect> SneakEffect;

	FActiveGameplayEffectHandle SneakEffectHandle;
};
