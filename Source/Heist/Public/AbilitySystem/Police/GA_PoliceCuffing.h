#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_PoliceCuffing.generated.h"

class AThiefCharacter;

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
	UPROPERTY()
	TObjectPtr<AThiefCharacter> TargetThief;
};
