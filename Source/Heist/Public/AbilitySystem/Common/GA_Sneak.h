#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_Sneak.generated.h"

/**
 * 홀드 중 슬금슬금 이동. 입력을 누르는 동안 State.Sneaking 태그를 유지한다.
 * ActivationPolicy = WhileInputActive
 *
 * 이동속도 감소는 AttributeSet 구현 후 GE로 교체 예정.
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
	// TODO: AttributeSet 구현 후 GE로 교체
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Sneak")
	float SneakMoveSpeed = 150.0f;

	float OriginalMoveSpeed = 0.0f;
};
