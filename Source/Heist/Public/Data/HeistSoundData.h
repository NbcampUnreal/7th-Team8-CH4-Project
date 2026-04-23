#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "HeistSoundData.generated.h"

class USoundBase;
class USoundAttenuation;
class USoundClass;

UENUM(BlueprintType)
enum class EHeistSoundPlayMode : uint8
{
	OneShot UMETA(DisplayName = "One Shot"),
	Looping UMETA(DisplayName = "Looping"),
	BGM     UMETA(DisplayName = "BGM")
};

UENUM(BlueprintType)
enum class EHeistSoundType : uint8
{
	// Movement & Action (One-Shot)
	Footstep_Thief	UMETA(DisplayName = "Footstep (Thief)"),
	Footstep_Police	UMETA(DisplayName = "Footstep (Police)"),
	ItemDrop		UMETA(DisplayName = "ItemDrop"),
	Kick			UMETA(DisplayName = "Kick (Thief Miss)"),
	Swing			UMETA(DisplayName = "Swing (Police Miss)"),
	Hit_Thief		UMETA(DisplayName = "Hit (Thief Success)"),
	Hit_Police		UMETA(DisplayName = "Hit (Police Success)"),

	// Continuous & Channeling (Looping)
	Carry		UMETA(DisplayName = "Carry"),
	Cuffing		UMETA(DisplayName = "Cuffing"),
	Heal		UMETA(DisplayName = "Heal"),
	Escort		UMETA(DisplayName = "Escort"),
	Voice		UMETA(DisplayName = "Voice"),
	Engine		UMETA(DisplayName = "Engine"),

	// BGM
	BGM_Game_Normal	UMETA(DisplayName = "BGM (Game Normal)"),
	BGM_Game_Chase	UMETA(DisplayName = "BGM (Game Chase)"),

	None UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FHeistSoundData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Audio")
	EHeistSoundType SoundType = EHeistSoundType::Footstep_Thief;

	// 사운드 재생 방식 (단발성 vs 루프)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Audio")
	EHeistSoundPlayMode PlayMode = EHeistSoundPlayMode::OneShot;

	// 실제 재생할 사운드 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Audio")
	TObjectPtr<USoundBase> SoundAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Audio")
	TObjectPtr<USoundClass> SoundClass;

	// 거리에 따른 소리 감쇠 세팅
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Audio")
	TObjectPtr<USoundAttenuation> AttenuationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heist|Audio", meta = (EditCondition = "PlayMode == EHeistSoundPlayMode::BGM"))
	float DefaultFadeDuration = 2.0f;

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
