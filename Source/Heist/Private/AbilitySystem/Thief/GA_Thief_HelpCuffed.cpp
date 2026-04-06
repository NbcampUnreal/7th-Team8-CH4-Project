// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Thief/GA_Thief_HelpCuffed.h"

#include "AbilitySystem/HeistTags_Ability.h"

UGA_Thief_HelpCuffed::UGA_Thief_HelpCuffed()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::WhileInputActive;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	AbilityTags.AddTag(HeistAbilityTags::Ability_Thief_HelpCuffed);
}

void UGA_Thief_HelpCuffed::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Thief_HelpCuffed::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Thief_HelpCuffed::OnChannelingCompleted()
{
	Super::OnChannelingCompleted();
}
