#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_Thief_Kick.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class AHeistCharacter;

/**
 *
 */
UCLASS()
class HEIST_API UGA_Thief_Kick : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Thief_Kick();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Ability")
	TObjectPtr<UAnimMontage> KickMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	TSubclassOf<UGameplayEffect> KnockbackEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	float KnockbackDuration = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	float StunDuration = 2.f;

	// LaunchCharacter 수평 속도 크기
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	float KnockbackLaunchSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	float KnockbackZVelocity = 150.f;

	void OnBackAttackHit(const FGameplayEventData& Payload); // non-dynamic
	void ApplyKickToTarget(AHeistCharacter* Target, UAbilitySystemComponent* TargetASC);

	UFUNCTION() void OnKickMontageCompleted();
	UFUNCTION() void OnKickMontageCancelled();
};
