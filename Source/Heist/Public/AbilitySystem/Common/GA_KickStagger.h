// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_KickStagger.generated.h"

/**
 * 피격자 측에서 실행되는 스태거 어빌리티.
 * Event_KickHit 수신 시 트리거 — GA_Thief_Kick이 StunGE 적용 후 발행.
 *
 * 담당:
 *   - LaunchCharacter (EventMagnitude = KnockbackForce, GA_Thief_Kick에서 전달)
 *   - KnockBack 섹션부터 몽타주 재생 (ASC 경유 → 복제됨)
 *   - 몽타주 완료 → EndAbility
 *
 * 담당하지 않음:
 *   - State_Stunned / ActionDisabled / MoveDisabled (StunGE 담당)
 *   - 피격자 회전 (GA_Thief_Kick 담당)
 *   - KnockBack→Stunned 전환 (몽타주 AnimNotify 담당)
 *   - Stunned→Outro 전환 (HitReactionComponent::OnStunnedTagChanged 담당)
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

	// EventMagnitude가 0일 때 사용할 기본 넉백 힘
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Ability")
	float DefaultKnockbackForce = 300.f;

private:
	// KnockBack → KnockBackOut 재생 완료 → Stunned 섹션 명시적 실행
	UFUNCTION() void OnKnockbackSectionDone();

	// Stunned 루프 → HitReactionComponent가 Outro로 전환 → 몽타주 종료 → EndAbility
	UFUNCTION() void OnStaggerMontageCompleted();
	UFUNCTION() void OnStaggerMontageCancelled();

	const FName Section_Knockback = FName("KnockBack");
	const FName Section_Stunned   = FName("Stunned");
};