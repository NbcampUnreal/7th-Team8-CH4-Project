
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_KickStagger.generated.h"

/**
 * 이 코드는 원래 넉백 몽타주 용도로 작성되었습니다만, 현재는 이용하지 않습니다. 
 * 그러나 구현 복잡도 부분에서 참조할 부분이 있고 추후 복구 및 재사용 가능성을 고려하여 더미로 남겨두었습니다.
 */

UCLASS()
class HEIST_API UGA_KickStagger : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_KickStagger();

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

protected:
	// KnockBack / Stunned / Outro 섹션을 포함한 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Ability")
	TObjectPtr<UAnimMontage> StaggerMontage;

	// Stun GE를 Kick 이 아니라 Stagger에서 처리
	// 피격자에게 적용할 GE (State_Stunned / ActionDisabled / MoveDisabled)
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	TSubclassOf<UGameplayEffect> StunGEClass;
	
	// Payload에 값이 없을 때 사용할 기본 넉백 거리(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Ability")
	float DefaultKnockbackDistance = 300.f;
	
private:
	// 몽타주 섹션명 — 에디터에서 조정 가능
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Ability")
	FName Section_Knockback = FName("KnockBack");

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Ability")
	FName Section_Stunned = FName("Stunned");

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Ability")
	FName Section_Outro = FName("Outro");
	
	// --- 런타임 ---
	bool  bCachedApplyStun   = false;
	float CachedStunDuration = 0.f;
	bool bTransitioningToOutro = false;

	FActiveGameplayEffectHandle StunEffectHandle;

	// --- Flow ---
	void StartKnockbackPhase(float KnockbackDistance);
	void StartStunnedPhase();
	void StartOutroPhase();
	float GetMontageSectionDuration(const UAnimMontage* Montage, FName SectionName) const;
	void ClearStunEffect();

	void TryInterruptEscortOnPolice();
	
	UFUNCTION() 
	void OnKnockbackDone();
	UFUNCTION()
	void OnStunnedDurationFinished();
	UFUNCTION() 
	void OnStaggerCompleted();
	UFUNCTION() 
	void OnStaggerCancelled();
	
};
