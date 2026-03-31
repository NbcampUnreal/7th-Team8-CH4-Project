#include "AbilitySystem/Common/GA_Sneak.h"

#include "Character/HeistTags_State.h"
#include "AbilitySystemComponent.h"

UGA_Sneak::UGA_Sneak()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::WhileInputActive;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_Sneak::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(ASC)) return;

	ASC->AddReplicatedLooseGameplayTag(HeistStateTags::State_Sneaking);

	if (IsValid(SneakEffect))
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(SneakEffect, 1.0f, EffectContext);
		SneakEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	}
}

void UGA_Sneak::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (IsValid(ASC))
	{
		ASC->RemoveReplicatedLooseGameplayTag(HeistStateTags::State_Sneaking);

		if (SneakEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(SneakEffectHandle);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
