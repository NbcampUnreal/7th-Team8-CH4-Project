#include "AbilitySystem/Police/GA_PoliceCuffing.h"

#include "Character/ThiefCharacter.h"
#include "Character/HeistTags_State.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "AbilitySystemComponent.h"

UGA_PoliceCuffing::UGA_PoliceCuffing()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnGameplayEvent;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	AbilityTags.AddTag(HeistAbilityTags::Ability_Police_Cuffing);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = HeistAbilityTags::Ability_Police_Cuffing;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_PoliceCuffing::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

	TargetThief = nullptr;

	// 상호작용 처리
	AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
	TargetThief = Cast<AThiefCharacter>(TargetActor);

	if (!IsValid(TargetThief))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartChanneling(FName("Cuffing"));
}

void UGA_PoliceCuffing::OnChannelingCompleted()
{
	if (HasAuthority(&CurrentActivationInfo) && IsValid(TargetThief))
	{
		UAbilitySystemComponent* TargetASC = TargetThief->GetAbilitySystemComponent();
		if (IsValid(TargetASC))
		{
			FGameplayEventData Payload;
			Payload.Instigator = GetAvatarActorFromActorInfo();
			Payload.Target = TargetThief;

			TargetASC->HandleGameplayEvent(HeistEventTags::Event_CuffingComplete, &Payload);

			// [테스트 코드] 도둑 파트가 없어서 경찰이 직접 태그 교체
			//TargetASC->RemoveLooseGameplayTag(HeistStateTags::State_Thief_Injured);
			//TargetASC->AddLooseGameplayTag(HeistStateTags::State_Thief_Cuffed);

			// Injured GE 제거 후 Cuffed GE 적용
			FGameplayEffectQuery InjuredQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
				FGameplayTagContainer(HeistStateTags::State_Thief_Injured));
			TargetASC->RemoveActiveEffects(InjuredQuery);

			if (IsValid(CuffedEffectClass))
			{
				FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(CuffedEffectClass, 1.f);
				TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PoliceCuffing::OnChannelingCancelled()
{
	TargetThief = nullptr;
}
