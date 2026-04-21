#include "AbilitySystem/Thief/GA_Thief_Depart.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "Core/HeistMatchGameMode.h"
#include "Actors/EscapeActor.h"

UGA_Thief_Depart::UGA_Thief_Depart()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnGameplayEvent;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	AbilityTags.AddTag(HeistAbilityTags::Ability_Thief_Depart);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = HeistAbilityTags::Ability_Thief_Depart;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Thief_Depart::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
		GroupIndex = Cast<AEscapeActor>(InteractedActor.Get())->EscapeGroupIndex;
	}

	StartChanneling(FName("Depart"));
}

void UGA_Thief_Depart::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Thief_Depart::OnChannelingCompleted()
{
	if (!HasAuthority(&CurrentActivationInfo)) return;

	if (AHeistMatchGameMode* HeistGM = GetWorld()->GetAuthGameMode<AHeistMatchGameMode>())
	{
		HeistGM->JudgeScore(GroupIndex);
	}
}
