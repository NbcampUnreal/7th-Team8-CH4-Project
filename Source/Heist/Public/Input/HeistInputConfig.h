#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "HeistInputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FHeistInputMapping
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> InputAction;
};

UCLASS(BlueprintType, Const)
class HEIST_API UHeistInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// Native 입력(Move 등)에서 태그로 InputAction을 조회한다.
	const UInputAction* FindNativeActionByTag(const FGameplayTag& InputTag) const;

	// 직접 핸들러에 바인딩되는 입력 (Move 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Native")
	TArray<FHeistInputMapping> NativeInputMappings;

	// ASC로 라우팅되는 어빌리티 입력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Ability")
	TArray<FHeistInputMapping> AbilityInputMappings;
};
