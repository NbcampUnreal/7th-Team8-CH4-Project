#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "HeistSoundData.generated.h"

class USoundBase;
class USoundAttenuation;

/**
 *
 */
UENUM(BlueprintType)
enum class EHeistSoundType : uint8
{
	Footstep   UMETA(DisplayName = "Footstep"),
	Carry      UMETA(DisplayName = "Carry"),
	ItemDrop   UMETA(DisplayName = "ItemDrop"),
	Cuffing    UMETA(DisplayName = "Cuffing"),
	Voice      UMETA(DisplayName = "Voice")
};

USTRUCT(BlueprintType)
struct FHeistSoundData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Audio")
	EHeistSoundType SoundType = EHeistSoundType::Footstep;

	// 실제 재생할 사운드 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Audio")
	TObjectPtr<USoundBase> SoundAsset;

	// 거리에 따른 소리 감쇠 세팅
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Audio")
	TObjectPtr<USoundAttenuation> AttenuationSettings;

	// 기본 탐지 반경 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Detection")
	float BaseRadius = 300.f;

	// 부상 상태 시 탐지 반경 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Detection")
	float InjuredRadiusMultiplier = 1.0f;

	// 물건 운반 시 탐지 반경 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Detection")
	float CarryingRadiusMultiplier = 1.0f;

	// 즉시 탐지 여부 (예: 물건 낙하)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Detection")
	bool bInstantAlert = false;

	// 경찰 탐지 핑 발생 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Detection")
	bool bAffectsPoliceAlert = true;
};
