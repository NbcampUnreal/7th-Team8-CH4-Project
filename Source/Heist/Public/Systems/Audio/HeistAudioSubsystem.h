#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/HeistSoundData.h"
#include "HeistAudioSubsystem.generated.h"

class UAudioComponent;
class UDataTable;
class USceneComponent;

/**
 * Heist 게임의 인게임 사운드 재생을 중앙에서 통제하는 월드 서브시스템
 * 단발성 사운드 최적화 및 15개의 AudioComponent Object Pooling을 통한 루프 사운드를 관리
 */
UCLASS()
class HEIST_API UHeistAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void PlayOneShotSound(EHeistSoundType SoundType, const FVector& Location);
	UAudioComponent* PlayLoopingSound(EHeistSoundType SoundType, USceneComponent* AttachToComponent);
	void StopLoopingSound(UAudioComponent* AudioComponentToStop);

protected:
	void LoadSoundDataTable();
	void InitializeAudioPool();

	const FHeistSoundData* GetSoundData(EHeistSoundType SoundType) const;
	UAudioComponent* GetFreeAudioComponent();

private:
	UPROPERTY()
	TObjectPtr<UDataTable> SoundDataTable;

	UPROPERTY()
	TArray<TObjectPtr<UAudioComponent>> AudioPool;

	static constexpr int32 MaxAudioPoolSize = 15;
	static constexpr float DefaultVolumeMultiplier = 1.0f;
	static constexpr float DefaultPitchMultiplier = 1.0f;
	static constexpr float DefaultFadeOutDuration = 0.2f;
};
