#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "HeistPawnData.generated.h"

class UHeistInputConfig;
class UHeistAbilitySet;
class UInputMappingContext;

/**
 * DT_CharacterStats DataTable의 행 구조체.
 * 캐릭터 종류(도둑/경찰)별로 행을 만들어 기본 스탯을 정의한다.
 */
USTRUCT(BlueprintType)
struct FHeistCharacterStatsRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float MaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	float MoveSpeed = 400.0f;
};

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UHeistAbilitySet> DefaultAbilitySet;

	// 기본 스탯 DataTable. FHeistCharacterStatsRow 행 타입을 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UDataTable> CharacterStatsTable;

	// CharacterStatsTable에서 읽을 행 이름 (예: "Thief", "Police")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	FName StatsRowName;
};
