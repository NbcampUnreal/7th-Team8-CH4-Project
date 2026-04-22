#include "Systems/Audio/HeistAudioSubsystem.h"

#include "Data/HeistSoundData.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"

void UHeistAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadSoundDataTable();
	InitializeAudioPool();
}

void UHeistAudioSubsystem::Deinitialize()
{
	AudioPool.Empty();
	SoundDataTable = nullptr;

	Super::Deinitialize();
}

void UHeistAudioSubsystem::LoadSoundDataTable()
{
	// TODO (하민): 현재는 경로 하드코딩 중 -> 추후 UDeveloperSettings 또는 GameInstance Config 로 분리
	const FString DataTablePath = TEXT("/Game/Heist/Data/DT_HeistSoundData.DT_HeistSoundData");
	SoundDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *DataTablePath));

	if (!IsValid(SoundDataTable))
	{
		UE_LOG(LogTemp, Error, TEXT("HeistAudioSubsystem: SoundDataTable을 로드할 수 없습니다. 경로 확인: %s"), *DataTablePath);
	}
}

const FHeistSoundData* UHeistAudioSubsystem::GetSoundData(EHeistSoundType SoundType) const
{
	if (!IsValid(SoundDataTable)) return nullptr;

	static const FString ContextString(TEXT("HeistAudioSubsystem::GetSoundData"));
	const UEnum* EnumPtr = StaticEnum<EHeistSoundType>();
	if (!IsValid(EnumPtr)) return nullptr;

	const FName RowName = FName(*EnumPtr->GetNameStringByValue(static_cast<int64>(SoundType)));
	return SoundDataTable->FindRow<FHeistSoundData>(RowName, ContextString);
}

void UHeistAudioSubsystem::InitializeAudioPool()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	for (int32 Index = 0; Index < MaxAudioPoolSize; ++Index)
	{
		UAudioComponent* NewAudioComp = NewObject<UAudioComponent>(World);
		if (IsValid(NewAudioComp))
		{
			NewAudioComp->bAutoDestroy = false;
			NewAudioComp->SetupAttachment(nullptr);
			NewAudioComp->RegisterComponentWithWorld(World);

			AudioPool.Add(NewAudioComp);
		}
	}
}

UAudioComponent* UHeistAudioSubsystem::GetFreeAudioComponent()
{
	for (UAudioComponent* AudioComp : AudioPool)
	{
		if (IsValid(AudioComp) && !AudioComp->IsPlaying())
		{
			return AudioComp;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("HeistAudioSubsystem: 오디오 풀(%d)이 가득 찼습니다!"), MaxAudioPoolSize);
	return nullptr;
}

void UHeistAudioSubsystem::PlayOneShotSound(EHeistSoundType SoundType, const FVector& Location)
{
	const FHeistSoundData* SoundData = GetSoundData(SoundType);
	if (SoundData == nullptr) return;

	if (SoundData->PlayMode != EHeistSoundPlayMode::OneShot)
	{
		UE_LOG(LogTemp, Warning, TEXT("HeistAudioSubsystem: PlayOneShotSound에 Looping 타입의 사운드가 전달되었습니다."));
	}

	if (IsValid(SoundData->SoundAsset))
	{
		UWorld* World = GetWorld();
		if (!IsValid(World)) return;

		UGameplayStatics::PlaySoundAtLocation(
			World,
			SoundData->SoundAsset,
			Location,
			DefaultVolumeMultiplier,
			DefaultPitchMultiplier,
			0.0f,
			SoundData->AttenuationSettings
		);
	}
}

UAudioComponent* UHeistAudioSubsystem::PlayLoopingSound(EHeistSoundType SoundType, USceneComponent* AttachToComponent)
{
	if (!IsValid(AttachToComponent)) return nullptr;

	const FHeistSoundData* SoundData = GetSoundData(SoundType);
	if (SoundData == nullptr) return nullptr;

	if (SoundData->PlayMode != EHeistSoundPlayMode::Looping)
	{
		UE_LOG(LogTemp, Warning, TEXT("HeistAudioSubsystem: PlayLoopingSound에 OneShot 타입의 사운드가 전달되었습니다."));
	}

	UAudioComponent* FreeAudioComp = GetFreeAudioComponent();
	if (!IsValid(FreeAudioComp)) return nullptr;

	if (IsValid(SoundData->SoundAsset))
	{
		FreeAudioComp->SetSound(SoundData->SoundAsset);
		FreeAudioComp->AttenuationSettings = SoundData->AttenuationSettings;

		FreeAudioComp->AttachToComponent(AttachToComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		FreeAudioComp->Play();

		return FreeAudioComp;
	}

	return nullptr;
}

void UHeistAudioSubsystem::StopLoopingSound(UAudioComponent* AudioComponentToStop)
{
	if (!IsValid(AudioComponentToStop)) return;

	if (AudioPool.Contains(AudioComponentToStop))
	{
		AudioComponentToStop->FadeOut(DefaultFadeOutDuration, 0.0f);
		AudioComponentToStop->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
}
