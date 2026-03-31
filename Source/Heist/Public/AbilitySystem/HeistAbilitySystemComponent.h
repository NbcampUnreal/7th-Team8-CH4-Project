#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "HeistAbilitySystemComponent.generated.h"

UCLASS()
class HEIST_API UHeistAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	// 입력 태그로 어빌리티 활성화를 요청한다.
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
};
