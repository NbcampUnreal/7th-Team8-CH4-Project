#include "AbilitySystem/Thief/GA_Thief_CloseDoor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "Actors/VehicleActor.h"
#include "Core/HeistMatchGameMode.h"

UGA_Thief_CloseDoor::UGA_Thief_CloseDoor()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnGameplayEvent;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	AbilityTags.AddTag(HeistAbilityTags::Ability_Thief_CloseDoor);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = HeistAbilityTags::Ability_Thief_CloseDoor;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Thief_CloseDoor::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	if (TriggerEventData && TriggerEventData->Target)
	{
		// 이벤트 데이터에서 EscapeActor를 추출
		TWeakObjectPtr<AActor> InteractedActor = const_cast<AActor*>(TriggerEventData->Target.Get());
		GroupIndex = Cast<AVehicleActor>(InteractedActor.Get())->EscapeGroupIndex;
	}

	StartChanneling(FName("CloseDoor"));
}

void UGA_Thief_CloseDoor::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Thief_CloseDoor::OnChannelingCompleted()
{
	if (!HasAuthority(&CurrentActivationInfo)) return;

	if (AHeistMatchGameMode* HeistGM = GetWorld()->GetAuthGameMode<AHeistMatchGameMode>())
	{
		if (!HeistGM->TryCheckDoorMoving(GroupIndex))
		{
			if (HeistGM->TryCheckDoorOpened(GroupIndex))	HeistGM->TryCloseDoor(GroupIndex);
			else HeistGM->TryOpenDoor(GroupIndex);
		}
	}
}
