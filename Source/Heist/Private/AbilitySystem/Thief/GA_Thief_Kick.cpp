// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Thief/GA_Thief_Kick.h"

#include "AbilitySystem/HeistTags_Ability.h"
#include "AbilitySystem/HeistTags_Data.h"
#include "Character/HeistCharacter.h"
#include "Character/HeistTags_State.h"
#include "Character/ThiefCharacter.h"
#include "Components/HeistHitReactionComponent.h"
#include "Components/ThiefEscortComponent.h"
#include "Components/HeistNoiseComponent.h"
#include "Data/HeistSoundData.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

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

	if (AThiefCharacter* Thief = Cast<AThiefCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UHeistNoiseComponent* NoiseComp = Thief->GetHeistNoiseComponent())
		{
			NoiseComp->MakeHeistNoise(EHeistSoundType::Kick, Thief->GetActorLocation());
		}
	}

	if (!IsValid(KickMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 현재 Kick의 hit 처리 함수를 HitReactionComponent에 등록합니다.
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (UHeistHitReactionComponent* HRC =
		IsValid(Avatar) ? Avatar->FindComponentByClass<UHeistHitReactionComponent>() : nullptr)
	{
		FHeistMeleeHitDelegate Handler;
		Handler.BindUObject(this, &UGA_Thief_Kick::OnBackAttackHit);
		HRC->SetMeleeHitHandler(Handler);
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

	// 방어코드
	if (TargetASC->HasMatchingGameplayTag(HeistStateTags::State_MoveDisabled)) return;
	if (TargetASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured)) return;
	if (TargetASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed)) return;
	if (TargetASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Escorted)) return;

	ApplyKickToTarget(Target, TargetASC);
}

void UGA_Thief_Kick::ApplyKickToTarget(AHeistCharacter* Target, UAbilitySystemComponent* TargetASC)
{
	if (!IsValid(Target) || !IsValid(TargetASC)) return;

	AActor* Self = GetAvatarActorFromActorInfo();
	if (!IsValid(Self)) return;

	if (UThiefEscortComponent::FindEscortedThiefByPolice(Target))
	{
		FGameplayTagContainer EscortAbilityTags;
		EscortAbilityTags.AddTag(HeistAbilityTags::Ability_Police_Escort);
		TargetASC->CancelAbilities(&EscortAbilityTags, nullptr, nullptr);
	}

	FVector LaunchDirection = Target->GetActorLocation() - Self->GetActorLocation();
	LaunchDirection.Z = 0.f;
	if (LaunchDirection.IsNearlyZero()) return;

	LaunchDirection = LaunchDirection.GetSafeNormal();
	Target->SetActorRotation(LaunchDirection.Rotation());

	const FVector LaunchVelocity = (LaunchDirection * KnockbackLaunchSpeed) + FVector::UpVector * KnockbackZVelocity; // 어우씨 이거 넣으니까 공중으로 뜬다
	Target->LaunchCharacter(LaunchVelocity, true, true);

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC)) return;

	if (IsValid(KnockbackEffectClass) && KnockbackDuration > 0.f)
	{
		FGameplayEffectSpecHandle KnockbackSpec = MakeOutgoingGameplayEffectSpec(KnockbackEffectClass, 1.f);
		if (KnockbackSpec.IsValid() && KnockbackSpec.Data.IsValid())
		{
			KnockbackSpec.Data->SetSetByCallerMagnitude(
				HeistDataTags::Data_Duration_Knockback,
				KnockbackDuration);
			TargetASC->ApplyGameplayEffectSpecToSelf(*KnockbackSpec.Data.Get());
		}
	}

	if (!IsValid(StunEffectClass) || StunDuration <= 0.f) return;

	UWorld* World = Target->GetWorld();
	if (!IsValid(World)) return;

	TWeakObjectPtr<UAbilitySystemComponent> WeakSourceASC(SourceASC);
	TWeakObjectPtr<UAbilitySystemComponent> WeakTargetASC(TargetASC);
	TSubclassOf<UGameplayEffect> LocalStunEffectClass = StunEffectClass;
	const float LocalStunDuration = StunDuration;
	FTimerDelegate ApplyStunDelegate;
	ApplyStunDelegate.BindLambda([WeakSourceASC, WeakTargetASC, LocalStunEffectClass, LocalStunDuration]()
		{
			if (!WeakSourceASC.IsValid() || !WeakTargetASC.IsValid() || !LocalStunEffectClass) return;

			FGameplayEffectContextHandle EffectContext = WeakSourceASC->MakeEffectContext();
			FGameplayEffectSpecHandle StunSpec = WeakSourceASC->MakeOutgoingSpec(LocalStunEffectClass, 1.f, EffectContext);
			if (!StunSpec.IsValid() || !StunSpec.Data.IsValid()) return;

			StunSpec.Data->SetSetByCallerMagnitude(
				HeistDataTags::Data_Duration_Stun,
				LocalStunDuration);
			WeakTargetASC->ApplyGameplayEffectSpecToSelf(*StunSpec.Data.Get());
		});

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, ApplyStunDelegate, KnockbackDuration, false);
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
		HRC->ResetMeleeHitHandler();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
