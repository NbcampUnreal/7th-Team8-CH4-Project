#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemData.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EItemGrade : uint8
{
	Small    UMETA(DisplayName = "Small"),
	Medium   UMETA(DisplayName = "Medium"),
	Large    UMETA(DisplayName = "Large"),
	Key      UMETA(DisplayName = "Key")
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	EItemGrade ItemGrade = EItemGrade::Small;

	UPROPERTY(EditDefaultsOnly)
	int32 RequiredCarriers = 1;

	UPROPERTY(EditDefaultsOnly)
	float CarrySpeedMultiplier = 0.85f;

	UPROPERTY(EditDefaultsOnly)
	float SoloCarrySpeedMultiplier = 0.85f;

	UPROPERTY(EditDefaultsOnly)
	float SoloCarryNoiseMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bTriggerGPS;

	UPROPERTY(EditDefaultsOnly)
	bool bTriggerAlarm;

	UPROPERTY(EditDefaultsOnly)
	int32 Value = 1;

	UPROPERTY(EditDefaultsOnly)
	bool bExplosive;

	UPROPERTY(EditDefaultsOnly)
	float ExplosionRadius = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	float ExplosionStunDuration = 2.0f;
};
