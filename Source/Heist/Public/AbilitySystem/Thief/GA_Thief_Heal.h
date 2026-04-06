// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_Thief_Heal.generated.h"

/**
 * 
 */
UCLASS()
class HEIST_API UGA_Thief_Heal : public UHeistGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Thief_Heal();
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;
	
};
