// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Police/GA_PoliceSwing.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "Character/HeistCharacter.h"
#include "Character/HeistTags_State.h"
#include "Components/HeistHitReactionComponent.h"

UGA_PoliceSwing::UGA_PoliceSwing()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 시전 중 이동 속도는 GE로 감소 (MoveDisabled 대신 느린 이동 허용)
	AbilityTags.AddTag(HeistAbilityTags::Ability_Police_Attack);
	ActivationOwnedTags.AddTag(HeistAbilityTags::Ability_Police_Attack);
	ActivationBlockedTags.AddTag(HeistAbilityTags::Ability_Police_Attack);
}

void UGA_PoliceSwing::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

	if (!IsValid(AttackMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 시전 중 이동 속도 감소 GE 적용
	if (IsValid(AttackSlowEffectClass))
	{
		FGameplayEffectSpecHandle SlowSpec = MakeOutgoingGameplayEffectSpec(AttackSlowEffectClass, 1.f);
		AttackSlowEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SlowSpec);
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_PoliceSwing::OnAttackMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_PoliceSwing::OnAttackMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_PoliceSwing::OnAttackMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_PoliceSwing::OnAttackMontageCancelled);
	MontageTask->ReadyForActivation();

	// UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
	// 	this, HeistEventTags::Event_Hit);
	// HitTask->EventReceived.AddDynamic(this, &UGA_PoliceSwing::OnHitEvent);
	// HitTask->ReadyForActivation();

	// 이 로직은 공격자의 Action을 HitReactionComponent에 캐싱해놓는 것이다.
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (UHeistHitReactionComponent* HRC =
		IsValid(Avatar) ? Avatar->FindComponentByClass<UHeistHitReactionComponent>() : nullptr)
	{
		HRC->OnMeleeHit.BindUObject(this, &UGA_PoliceSwing::OnHitEvent);
	}
}

void UGA_PoliceSwing::OnHitEvent(const FGameplayEventData& Payload)
{
	AHeistCharacter* Target = Cast<AHeistCharacter>(const_cast<AActor*>(Payload.Target.Get()));
	if (!IsValid(Target)) return;
	
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (!IsValid(TargetASC)) return;
	
	// Cuffed, Injured, Escorted 상태 도둑은 피격 무시
	if (TargetASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed)) return;
	if (TargetASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured)) return;
	if (TargetASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Escorted)) return;

	if (!HasAuthority(&CurrentActivationInfo)) return;

	if (IsValid(InjuredEffectClass))
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(InjuredEffectClass, 1.f);
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UGA_PoliceSwing::OnAttackMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PoliceSwing::OnAttackMontageCancelled()
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void UGA_PoliceSwing::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 이동 속도 감소 GE 해제
	if (AttackSlowEffectHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(AttackSlowEffectHandle);
		AttackSlowEffectHandle.Invalidate();
	}
	
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (UHeistHitReactionComponent* HRC =
		IsValid(Avatar) ? Avatar->FindComponentByClass<UHeistHitReactionComponent>() : nullptr)
	{
		HRC->OnMeleeHit.Unbind();
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}