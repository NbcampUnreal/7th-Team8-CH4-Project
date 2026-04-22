#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_PoliceCuffing.generated.h"

class AThiefCharacter;
class UAudioComponent;

UCLASS()
class HEIST_API UGA_PoliceCuffing : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PoliceCuffing();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void OnChannelingCompleted() override;
	virtual void OnChannelingCancelled() override;

private:
	// 수갑 완료시 도둑에게 부착할 GE (State_Thief_Cuffed 부여)
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	TSubclassOf<UGameplayEffect> CuffedEffectClass;

	UPROPERTY()
	TObjectPtr<AThiefCharacter> TargetThief;

	UPROPERTY()
	TObjectPtr<UAudioComponent> CuffingAudioComp;
};
