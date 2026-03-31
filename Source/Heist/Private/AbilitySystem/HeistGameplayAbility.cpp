#include "AbilitySystem/HeistGameplayAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

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

	UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, Data->Duration);
	WaitTask->OnFinish.AddDynamic(this, &UHeistGameplayAbility::OnChannelingTimerExpired);
	WaitTask->ReadyForActivation();
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
