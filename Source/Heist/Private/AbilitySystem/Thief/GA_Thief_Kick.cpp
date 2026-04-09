// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Thief/GA_Thief_Kick.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "AbilitySystem/HeistTags_Data.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "Character/HeistCharacter.h"
#include "Character/HeistTags_State.h"
#include "Components/HeistHitReactionComponent.h"

UGA_Thief_Kick::UGA_Thief_Kick()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	ActivationOwnedTags.AddTag(HeistStateTags::State_MoveDisabled);
	AbilityTags.AddTag(HeistAbilityTags::Ability_Thief_Kick);
	ActivationOwnedTags.AddTag(HeistAbilityTags::Ability_Thief_Kick);
	ActivationBlockedTags.AddTag(HeistAbilityTags::Ability_Thief_Kick);

}

void UGA_Thief_Kick::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

	if (!IsValid(KickMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// 이 로직은 공격자의 Action을 HitReactionComponent에 캐싱해놓는 것이다.
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (UHeistHitReactionComponent* HRC =
		IsValid(Avatar) ? Avatar->FindComponentByClass<UHeistHitReactionComponent>() : nullptr)
	{
		HRC->OnMeleeHit.BindUObject(this, &UGA_Thief_Kick::OnBackAttackHit);
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, KickMontage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Thief_Kick::OnKickMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Thief_Kick::OnKickMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Thief_Kick::OnKickMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Thief_Kick::OnKickMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_Thief_Kick::OnBackAttackHit(const FGameplayEventData& Payload)
{
	if (!HasAuthority(&CurrentActivationInfo)) return;

	AHeistCharacter* Target = Cast<AHeistCharacter>(const_cast<AActor*>(Payload.Target.Get()));
	if (!IsValid(Target)) return;

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (!IsValid(TargetASC)) return;

	// 공격자 반대 방향으로 회전 (배쪽 = 넉백 방향)
	AActor* Self = GetAvatarActorFromActorInfo();
	if (IsValid(Self))
	{
		FVector Direction = Target->GetActorLocation() - Self->GetActorLocation();
		Direction.Z = 0.f;
		if (!Direction.IsNearlyZero())
			Target->SetActorRotation(Direction.GetSafeNormal().Rotation());
	}
	
	// GE_Stunned 적용 -> State_Stunned 부여 -> GA_PoliceEscort가 자동 취소되는 흐름입니다.
	if (IsValid(StunGEClass))
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(StunGEClass, 1.f);
		Spec.Data->SetSetByCallerMagnitude(HeistDataTags::Data_Duration_Stun, StunDuration);
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
	
	// GA_KickStagger 트리거 — 피격자 ASC에 이벤트 발행
	// EventMagnitude로 넉백 힘 전달 → GA_KickStagger::ActivateAbility에서 LaunchCharacter에 사용
	FGameplayEventData KickPayload;
	KickPayload.Instigator = GetAvatarActorFromActorInfo();
	KickPayload.Target = Target;
	KickPayload.EventMagnitude = KnockbackForce;
	TargetASC->HandleGameplayEvent(HeistEventTags::Event_KickHit, &KickPayload);
}

void UGA_Thief_Kick::OnKickMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Thief_Kick::OnKickMontageCancelled()
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void UGA_Thief_Kick::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (UHeistHitReactionComponent* HRC =
		IsValid(Avatar) ? Avatar->FindComponentByClass<UHeistHitReactionComponent>() : nullptr)
	{
		HRC->OnMeleeHit.Unbind();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}