#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/HeistSoundData.h"
#include "HeistAudioSubsystem.generated.h"

class UAudioComponent;
class UDataTable;
class USceneComponent;
class USoundMix;
class USoundClass;

/**
 * Heist 게임의 인게임 사운드 재생을 중앙에서 통제하는 월드 서브시스템
 * 단발성 사운드 최적화 및 15개의 AudioComponent Object Pooling을 통한 루프 사운드를 관리
 */
UENUM(BlueprintType)
enum class EHeistAudioBus : uint8
{
	Master,
	BGM,
	SFX
};

UCLASS()
class HEIST_API UHeistAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void PlayOneShotSound(EHeistSoundType SoundType, const FVector& Location);
	UAudioComponent* PlayLoopingSound(EHeistSoundType SoundType, USceneComponent* AttachToComponent);
	void StopLoopingSound(UAudioComponent* AudioComponentToStop);

	void TransitionToBGM(EHeistSoundType NewBGMType, float OverrideFadeDuration = NoFadeDurationOverride, bool bStartFromBeginning = true);
	void ApplyBaseSoundMix();

	UFUNCTION(BlueprintCallable, Category = "Heist|Audio")
	void SetBusVolume(EHeistAudioBus Bus, float NewVolume);

	float GetBusVolume(EHeistAudioBus Bus) const;

	static constexpr float NoFadeDurationOverride = -1.0f;

protected:
	void LoadSoundDataTable();
	void InitializeAudioPool();
	void InitializeBGMComponents();

	const FHeistSoundData* GetSoundData(EHeistSoundType SoundType) const;
	UAudioComponent* GetFreeAudioComponent();
	USoundClass* GetSoundClassFromBus(EHeistAudioBus Bus) const;

private:
	UPROPERTY()
	TObjectPtr<UDataTable> SoundDataTable;

	UPROPERTY()
	TArray<TObjectPtr<UAudioComponent>> AudioPool;

	UPROPERTY()
	TObjectPtr<UAudioComponent> BGMComponentActive;

	UPROPERTY()
	TObjectPtr<UAudioComponent> BGMComponentStandby;

	UPROPERTY()
	TObjectPtr<USoundMix> BaseSoundMix;

	UPROPERTY()
	TObjectPtr<USoundClass> ClassMaster;

	UPROPERTY()
	TObjectPtr<USoundClass> ClassBGM;

	UPROPERTY()
	TObjectPtr<USoundClass> ClassSFX;

	EHeistSoundType CurrentBGMType = EHeistSoundType::None;

	bool bIsComponentAActive = true;

	static constexpr int32 MaxAudioPoolSize = 15;
	static constexpr float DefaultVolumeMultiplier = 1.0f;
	static constexpr float DefaultPitchMultiplier = 1.0f;
	static constexpr float DefaultFadeOutDuration = 0.2f;

	static constexpr float SoundMixPitchMultiplier = 1.0f;
	static constexpr float SoundMixVolumeFadeTime = 0.1f;

	float MasterVolume = 1.0f;
	float BGMVolume = 1.0f;
	float SFXVolume = 1.0f;
};
