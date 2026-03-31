#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "HeistAbilitySystemComponent.generated.h"

UCLASS()
class HEIST_API UHeistAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	static UHeistAbilitySystemComponent* FindAbilitySystemComponent(const AActor* Actor);

	// 입력 태그를 큐에 적재한다. ProcessAbilityInput에서 처리된다.
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	// [ALL] PlayerController::PlayerTick에서 호출. 큐에 쌓인 입력을 처리한다.
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

private:
	TArray<FGameplayTag> InputPressedTags;
	TArray<FGameplayTag> InputReleasedTags;
};
