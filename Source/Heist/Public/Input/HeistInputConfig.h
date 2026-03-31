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
	const UInputAction* FindInputActionByTag(const FGameplayTag& InputTag) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TArray<FHeistInputMapping> InputMappings;
};
