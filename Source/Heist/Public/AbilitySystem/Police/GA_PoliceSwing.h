// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_PoliceSwing.generated.h"

/**
 * 
 */
UCLASS()
class HEIST_API UGA_PoliceSwing : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PoliceSwing();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Ability")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 도둑에게 부착할 GE (State_Thief_Injured 부여)
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	TSubclassOf<UGameplayEffect> InjuredEffectClass;

	// 시전 중 이동 속도 감소 GE (완전 봉인 대신 느린 이동)
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	TSubclassOf<UGameplayEffect> AttackSlowEffectClass;

	FActiveGameplayEffectHandle AttackSlowEffectHandle;

	UFUNCTION()
	void OnHitEvent(const FGameplayEventData& Payload);

	UFUNCTION()
	void OnAttackMontageCompleted();

	UFUNCTION()
	void OnAttackMontageCancelled();
};
