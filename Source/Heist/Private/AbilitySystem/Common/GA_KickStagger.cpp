// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Common/GA_KickStagger.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "Character/HeistTags_State.h"
#include "GameFramework/Character.h"

UGA_KickStagger::UGA_KickStagger()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnInputTriggered; 
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true; // 재시작 허용
	
	// GA_Thief_Kick::OnBackAttackHit → 피격자 ASC에 Event_KickHit 발행 → 트리거
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = HeistEventTags::Event_KickHit;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 베이스 클래스가 추가한 ActionDisabled 차단 제거 — 피격자는 스턴 중에도 활성화되어야 함
	ActivationBlockedTags.RemoveTag(HeistStateTags::State_ActionDisabled);
	ActivationBlockedTags.RemoveTag(HeistStateTags::State_Stunned);
}

void UGA_KickStagger::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(StaggerMontage) || !IsValid(Avatar))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// GA_Thief_Kick이 EventMagnitude로 넉백 힘을 전달, 없으면 기본값 사용
	const float KnockbackForce = (TriggerEventData && TriggerEventData->EventMagnitude > 0.f)
		? TriggerEventData->EventMagnitude
		: DefaultKnockbackForce;

	// 공격자 → 피격자 방향으로 넉백 (GA_Thief_Kick이 이미 회전을 처리했으므로 ForwardVector 사용)
	const FVector KnockbackDir = Avatar->GetActorForwardVector();
	Avatar->LaunchCharacter(KnockbackDir * KnockbackForce, true, false);

	// Phase 1: KnockBack → KnockBackOut 재생 (KnockBack→KnockBackOut 자동 연결)
	// KnockBackOut 종료 시 OnCompleted → OnKnockbackSectionDone → Phase 2 진입
	UAbilityTask_PlayMontageAndWait* KnockbackTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, FName("KnockbackTask"), StaggerMontage, 1.f, Section_Knockback);
	KnockbackTask->OnCompleted.AddDynamic(this, &UGA_KickStagger::OnKnockbackSectionDone);
	KnockbackTask->OnInterrupted.AddDynamic(this, &UGA_KickStagger::OnStaggerMontageCancelled);
	KnockbackTask->OnCancelled.AddDynamic(this, &UGA_KickStagger::OnStaggerMontageCancelled);
	KnockbackTask->ReadyForActivation();
}

void UGA_KickStagger::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_KickStagger::OnKnockbackSectionDone()
{
	// Phase 2: Stunned 루프 재생
	// Stunned→Outro 전환: HitReactionComponent::OnStunnedTagChanged(Count==0) 처리
	// Outro 종료 → OnCompleted → EndAbility
	UAbilityTask_PlayMontageAndWait* StunnedTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, FName("StunnedTask"), StaggerMontage, 1.f, Section_Stunned);
	StunnedTask->OnCompleted.AddDynamic(this, &UGA_KickStagger::OnStaggerMontageCompleted);
	StunnedTask->OnBlendOut.AddDynamic(this, &UGA_KickStagger::OnStaggerMontageCompleted);
	StunnedTask->OnInterrupted.AddDynamic(this, &UGA_KickStagger::OnStaggerMontageCancelled);
	StunnedTask->OnCancelled.AddDynamic(this, &UGA_KickStagger::OnStaggerMontageCancelled);
	StunnedTask->ReadyForActivation();
}

void UGA_KickStagger::OnStaggerMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_KickStagger::OnStaggerMontageCancelled()
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}