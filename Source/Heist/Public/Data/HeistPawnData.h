#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HeistPawnData.generated.h"

class UHeistInputConfig;
class UInputMappingContext;

UCLASS(BlueprintType, Const)
class HEIST_API UHeistPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 DefaultMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UHeistInputConfig> InputConfig;

	// TODO: AbilitySet 추가 예정
};
