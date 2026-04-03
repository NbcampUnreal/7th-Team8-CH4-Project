#include "AbilitySystem/HeistGameplayAbility.h"

#include "AbilitySystem/HeistTags_Event.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

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

void UHeistGameplayAbility::OnChannelingTimerExpired()
{
	bIsChanneling = false;
	OnChannelingCompleted();
}

void UHeistGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bIsChanneling && bWasCancelled)
	{
		bIsChanneling = false;
		OnChannelingCancelled();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
