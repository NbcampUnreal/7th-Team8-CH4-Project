
#include "AbilitySystem/Common/GA_KickStagger.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/HeistTags_Data.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "Animation/AnimMontage.h"
#include "Character/HeistCharacter.h"
#include "Character/HeistTags_State.h"
#include "Character/ThiefCharacter.h"
#include "Components/ThiefEscortComponent.h"
#include "GameFramework/Character.h"
#include "Math/UnrealMathUtility.h"

UGA_KickStagger::UGA_KickStagger()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnGameplayEvent;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true; // 재시작 허용
	
	// GA_Thief_Kick::OnBackAttackHit > 피격자 ASC에 Event_KickHit 발행 > 트리거
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = HeistEventTags::Event_KickHit;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 피격자는 스턴 GE를 이 GA가 직접 적용하므로 ActionDisabled 차단 제거
	ActivationBlockedTags.RemoveTag(HeistStateTags::State_ActionDisabled);
	
	// 이미 못 움직이는 대상은 발동 불가 -> 중첩 방지
	ActivationBlockedTags.AddTag(HeistStateTags::State_MoveDisabled);
	ActivationBlockedTags.AddTag(HeistStateTags::State_Thief_Injured);
	ActivationBlockedTags.AddTag(HeistStateTags::State_Thief_Cuffed);
	ActivationBlockedTags.AddTag(HeistStateTags::State_Thief_Escorted);
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

	// bRetriggerInstancedAbility 재진입 대비 delegate 정리
	if (HasAuthority(&CurrentActivationInfo))
	{
		ClearStunEffect();
	}

	// FHeistKickPayload에서 KnockbackDistance / StunDuration 파싱
	float KnockbackDistance = DefaultKnockbackDistance;
	bCachedApplyStun      = false;
	CachedStunDuration    = 0.f;
	bTransitioningToOutro = false;

	if (TriggerEventData && TriggerEventData->TargetData.IsValid(0))
	{
		const FHeistKickPayload* KickData =
			static_cast<const FHeistKickPayload*>(TriggerEventData->TargetData.Get(0));
		if (KickData)
		{
			KnockbackDistance  = KickData->KnockbackDistance;
			CachedStunDuration = KickData->StunDuration;
			bCachedApplyStun   = CachedStunDuration > 0.f;
		}
	}
	
	if (HasAuthority(&CurrentActivationInfo))
	{
		// 경찰이 Escort 중이었다면 Kick 맞는 즉시 도둑 탈출 처리
		TryInterruptEscortOnPolice();

		// KickStagger 시작 시 피해자의 진행 방향을 발차기 시전자 기준으로 고정한다.
		// 그래야 커서 방향과 무관하게 넉백이 "앞으로" 밀리며, 뒷걸음질처럼 보이지 않는다.
		if (TriggerEventData)
		{
			AActor* InstigatorActor = const_cast<AActor*>(TriggerEventData->Instigator.Get());
			if (IsValid(InstigatorActor) && IsValid(Avatar))
			{
				FVector KnockbackDirection = Avatar->GetActorLocation() - InstigatorActor->GetActorLocation();
				KnockbackDirection.Z = 0.f;
				if (!KnockbackDirection.IsNearlyZero())
				{
					Avatar->SetActorRotation(KnockbackDirection.GetSafeNormal().Rotation());
				}
			}
		}

		// KickStagger 전 구간 동안 Infinite StunGE를 유지한다.
		if (bCachedApplyStun)
		{
			if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
			{
				if (IsValid(StunGEClass))
				{
					FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(StunGEClass, 1.f);
					if (Spec.IsValid() && Spec.Data.IsValid())
					{
						StunEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
					}
				}
			}
		}
	}

	StartKnockbackPhase(KnockbackDistance);
}

void UGA_KickStagger::StartKnockbackPhase(float KnockbackDistance)
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Avatar))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const float KnockbackDuration = GetMontageSectionDuration(StaggerMontage, Section_Knockback);
	if (KnockbackDistance > 0.f && KnockbackDuration > KINDA_SMALL_NUMBER)
	{
		const float KnockbackSpeed = KnockbackDistance / KnockbackDuration;

		UAbilityTask_ApplyRootMotionConstantForce* ForceTask =
			UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
				this,
				FName("KickKnockbackForce"),
				Avatar->GetActorForwardVector(),
				KnockbackSpeed,
				KnockbackDuration,
				false,
				nullptr,
				ERootMotionFinishVelocityMode::SetVelocity,
				FVector::ZeroVector,
				0.f,
				false);
		ForceTask->ReadyForActivation();
	}

	// KnockBack phase 전환은 기존처럼 몽타주 task가 결정하고,
	// 실제 이동만 Knockback 섹션 길이만큼 RootMotionForce로 분리 적용합니다.
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, FName("KnockbackTask"), StaggerMontage, 1.f, Section_Knockback);
	Task->OnCompleted.AddDynamic(this, &UGA_KickStagger::OnKnockbackDone);
	Task->OnBlendOut.AddDynamic(this, &UGA_KickStagger::OnKnockbackDone);
	Task->OnInterrupted.AddDynamic(this, &UGA_KickStagger::OnStaggerCancelled);
	Task->OnCancelled.AddDynamic(this, &UGA_KickStagger::OnStaggerCancelled);
	Task->ReadyForActivation();
}

float UGA_KickStagger::GetMontageSectionDuration(const UAnimMontage* Montage, FName SectionName) const
{
	if (!IsValid(Montage) || SectionName.IsNone())
	{
		return 0.f;
	}

	const int32 SectionIndex = Montage->GetSectionIndex(SectionName);
	if (SectionIndex == INDEX_NONE)
	{
		return 0.f;
	}

	const float SectionStartTime = Montage->GetAnimCompositeSection(SectionIndex).GetTime();

	float SectionEndTime = Montage->GetPlayLength();
	const int32 NextSectionIndex = SectionIndex + 1;
	if (NextSectionIndex < Montage->GetNumSections())
	{
		SectionEndTime = Montage->GetAnimCompositeSection(NextSectionIndex).GetTime();
	}

	return FMath::Max(0.f, SectionEndTime - SectionStartTime);
}

void UGA_KickStagger::OnKnockbackDone()
{
	// KnockBack Only — 스턴 없이 바로 종료
	if (!bCachedApplyStun)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	StartStunnedPhase();
}

void UGA_KickStagger::StartStunnedPhase()
{
	// Stunned 섹션은 몽타주에서 루프 설정 필수
	// 실제 기절 시간은 GE duration이 아니라 GA가 직접 관리한다.
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, FName("StunnedTask"), StaggerMontage, 1.f, Section_Stunned);
	Task->OnCompleted.AddDynamic(this, &UGA_KickStagger::OnStaggerCompleted);
	Task->OnBlendOut.AddDynamic(this, &UGA_KickStagger::OnStaggerCompleted);
	Task->OnInterrupted.AddDynamic(this, &UGA_KickStagger::OnStaggerCancelled);
	Task->OnCancelled.AddDynamic(this, &UGA_KickStagger::OnStaggerCancelled);
	Task->ReadyForActivation();

	if (CachedStunDuration <= 0.f)
	{
		OnStunnedDurationFinished();
		return;
	}

	UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, CachedStunDuration);
	WaitTask->OnFinish.AddDynamic(this, &UGA_KickStagger::OnStunnedDurationFinished);
	WaitTask->ReadyForActivation();
}

void UGA_KickStagger::OnStunnedDurationFinished()
{
	if (!IsActive()) return;

	bTransitioningToOutro = true;
	StartOutroPhase();
}

void UGA_KickStagger::TryInterruptEscortOnPolice()
{
	if (!HasAuthority(&CurrentActivationInfo)) return;
	
	AHeistCharacter* Self = Cast<AHeistCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Self))
	{
		return;
	}
	
	// 도둑들의 EscortComponent를 조회하고 나를 Escort 경찰로 참조하는 대상 찾기
	AThiefCharacter* EscortedThief = UThiefEscortComponent::FindEscortedThiefByPolice(Self);
	if (!IsValid(EscortedThief)) return;
	
	UAbilitySystemComponent* PoliceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(PoliceASC)) return;

	FGameplayTagContainer EscortAbilityTags;
	EscortAbilityTags.AddTag(HeistAbilityTags::Ability_Police_Escort);
	PoliceASC->CancelAbilities(&EscortAbilityTags, nullptr, nullptr);
}

void UGA_KickStagger::StartOutroPhase()
{
	// PlayMontageAndWait → ASC->PlayMontage() 경유 → 서버→클라 복제 보장
	// 이전 StunnedTask는 새 Task 시작 시 자동 취소됨
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, FName("OutroTask"), StaggerMontage, 1.f, Section_Outro);
	Task->OnCompleted.AddDynamic(this, &UGA_KickStagger::OnStaggerCompleted);
	Task->OnBlendOut.AddDynamic(this, &UGA_KickStagger::OnStaggerCompleted);
	Task->OnInterrupted.AddDynamic(this, &UGA_KickStagger::OnStaggerCancelled);
	Task->OnCancelled.AddDynamic(this, &UGA_KickStagger::OnStaggerCancelled);
	Task->ReadyForActivation();
}

void UGA_KickStagger::OnStaggerCompleted()
{
	bTransitioningToOutro = false;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_KickStagger::OnStaggerCancelled()
{
	if (bTransitioningToOutro)
	{
		return;
	}

	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void UGA_KickStagger::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HasAuthority(&ActivationInfo))
	{
		ClearStunEffect();
	}
	bCachedApplyStun   = false;
	CachedStunDuration = 0.f;
	bTransitioningToOutro = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_KickStagger::ClearStunEffect()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (StunEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(StunEffectHandle);
		}
	}
	
	StunEffectHandle.Invalidate();
}
