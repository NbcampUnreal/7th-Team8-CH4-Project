#include "AbilitySystem/HeistGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/HeistTags_Event.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/HeistTags_State.h"

UHeistGameplayAbility::UHeistGameplayAbility()
{
	ActivationBlockedTags.AddTag(HeistStateTags::State_ActionDisabled);
}

const FChannelingData* UHeistGameplayAbility::GetChannelingData(FName RowName) const
{
	if (!IsValid(ChannelingDataTable)) return nullptr;
	return ChannelingDataTable->FindRow<FChannelingData>(RowName, TEXT("GetChannelingData"));
}

void UHeistGameplayAbility::StartChanneling(FName RowName)
{
	const FChannelingData* Data = GetChannelingData(RowName);
	if (Data == nullptr) return;

	bIsChanneling = true;

	// 0. 애니메이션 몽타주 재생 (선택 사항)
	if (IsValid(Data->ChannelingMontage))
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, Data->ChannelingMontage);

		// 몽타주의 재생이 완전히 끝났거나 중단되었을 때의 콜백 등록
		MontageTask->OnCompleted.AddDynamic(this, &UHeistGameplayAbility::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UHeistGameplayAbility::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UHeistGameplayAbility::OnMontageCancelled);
		MontageTask->OnCancelled.AddDynamic(this, &UHeistGameplayAbility::OnMontageCancelled);

		MontageTask->ReadyForActivation();
	}
	
	// Before - 채널링 잠금 GE 적용
	if (IsValid(ChannelingEffectClass))
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(ChannelingEffectClass, 1.f);
		ChannelingEffectHandle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, Spec);
	}
	
	// 1. 기본 타이머 (Duration)
	UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, Data->Duration);
	WaitTask->OnFinish.AddDynamic(this, &UHeistGameplayAbility::OnChannelingTimerExpired);
	WaitTask->ReadyForActivation();

	// 2. 외부 중단 태그 감지 (InterruptEventTag)
	if (Data->InterruptEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, Data->InterruptEventTag);
		EventTask->EventReceived.AddDynamic(this, &UHeistGameplayAbility::OnChannelingInterruptEvent);
		EventTask->ReadyForActivation();
	}

	// 3. 피격 시 취소 감지 (bCancelOnHit)
	if (Data->bCancelOnHit && Data->InterruptEventTag != HeistEventTags::Event_Hit)
	{
		UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HeistEventTags::Event_Hit);
		HitTask->EventReceived.AddDynamic(this, &UHeistGameplayAbility::OnChannelingInterruptEvent);
		HitTask->ReadyForActivation();
	}

	// 4. 이동 시 취소 감지 (bCancelOnMove)
	if (Data->bCancelOnMove)
	{
		UAbilityTask_WaitGameplayEvent* MoveTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HeistEventTags::Event_Input_Move);
		MoveTask->EventReceived.AddDynamic(this, &UHeistGameplayAbility::OnChannelingInterruptEvent);
		MoveTask->ReadyForActivation();
	}
}

void UHeistGameplayAbility::OnChannelingInterruptEvent(FGameplayEventData Payload)
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void UHeistGameplayAbility::OnMontageCompleted()
{
	// Outro 애니메이션이 끝나면 정상적으로 어빌리티를 종료합니다.
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UHeistGameplayAbility::OnMontageCancelled()
{
	// 몽타주가 비정상적으로 끊기면 취소 처리
	if (IsActive())
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

void UHeistGameplayAbility::OnChannelingTimerExpired()
{
	bIsChanneling = false;
	
	// 현재 재생 중인 몽타주가 있다면 Outro 섹션으로 강제 점프시킵니다.
	if (GetCurrentMontage())
	{
		MontageJumpToSection(FName("Outro"));
	}
	else
	{
		// 몽타주가 아예 없는 스킬이라면 타이머 종료 시 여기서 어빌리티를 직접 종료시킵니다.
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
	
	// 어빌리티 종료는 나중에 OnMontageCompleted 에서 하더라도, 
	// 스킬 발동 효과(데미지 판정, 투사체 발사 등)는 이 시점에서 터지도록 합니다.
	OnChannelingCompleted();
}

void UHeistGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 채널링 잠금 GE가 있다면 항상 제거한다
	if (ChannelingEffectHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(ChannelingEffectHandle);
		ChannelingEffectHandle.Invalidate();
	}
	
	if (bIsChanneling && bWasCancelled)
	{
		bIsChanneling = false;
		OnChannelingCancelled();
		// Montage는 GAS에서 자동 종료
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
