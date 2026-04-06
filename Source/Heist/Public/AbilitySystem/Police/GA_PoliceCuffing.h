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

	// TODO(하민): Interaction 컴포넌트 완성 시 트리거 이벤트 데이터로 타겟을 넘겨받고 이 변수는 삭제 예정
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Cuffing")
	float InteractRadius = 150.0f;
};
