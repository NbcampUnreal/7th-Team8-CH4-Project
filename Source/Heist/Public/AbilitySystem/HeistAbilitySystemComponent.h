#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "HeistAbilitySystemComponent.generated.h"

UCLASS()
class HEIST_API UHeistAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	// 입력 태그와 매칭되는 어빌리티를 활성화/종료한다.
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
};
