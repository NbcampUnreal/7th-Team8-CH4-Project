#include "Systems/Audio/HeistAudioSubsystem.h"

#include "Data/HeistSoundData.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"

bool UHeistAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;

	UWorld* World = Cast<UWorld>(Outer);
	return IsValid(World) && !World->IsNetMode(NM_DedicatedServer);
}

void UHeistAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ClassMaster = Cast<USoundClass>(StaticLoadObject(USoundClass::StaticClass(), nullptr, TEXT("/Game/Heist/Audio/Classes/SC_Master.SC_Master")));
	ClassBGM = Cast<USoundClass>(StaticLoadObject(USoundClass::StaticClass(), nullptr, TEXT("/Game/Heist/Audio/Classes/SC_BGM.SC_BGM")));
	ClassSFX = Cast<USoundClass>(StaticLoadObject(USoundClass::StaticClass(), nullptr, TEXT("/Game/Heist/Audio/Classes/SC_SFX.SC_SFX")));

	LoadSoundDataTable();
	InitializeAudioPool();
	InitializeBGMComponents();
}

void UHeistAudioSubsystem::Deinitialize()
{
	for (UAudioComponent* AudioComponent : AudioPool)
	{
		if (IsValid(AudioComponent))
		{
			AudioComponent->Stop();
			AudioComponent->UnregisterComponent();
		}
	}
	AudioPool.Empty();

	if (IsValid(BGMComponentActive))
	{
		BGMComponentActive->Stop();
		BGMComponentActive->UnregisterComponent();
		BGMComponentActive = nullptr;
	}

	if (IsValid(BGMComponentStandby))
	{
		BGMComponentStandby->Stop();
		BGMComponentStandby->UnregisterComponent();
		BGMComponentStandby = nullptr;
	}

	SoundDataTable = nullptr;
	BaseSoundMix = nullptr;

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

	const FString SoundMixPath = TEXT("/Game/Heist/Audio/Mix/SM_HeistBase.SM_HeistBase");
	BaseSoundMix = Cast<USoundMix>(StaticLoadObject(USoundMix::StaticClass(), nullptr, *SoundMixPath));
}

void UHeistAudioSubsystem::InitializeAudioPool()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	for (int32 Index = 0; Index < MaxAudioPoolSize; ++Index)
	{
		UAudioComponent* NewAudioComp = NewObject<UAudioComponent>(World, UAudioComponent::StaticClass());
		if (IsValid(NewAudioComp))
		{
			NewAudioComp->bAutoDestroy = false;
			NewAudioComp->SetupAttachment(nullptr);
			NewAudioComp->RegisterComponentWithWorld(World);

			AudioPool.Add(NewAudioComp);
		}
	}
}

void UHeistAudioSubsystem::InitializeBGMComponents()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	BGMComponentActive = NewObject<UAudioComponent>(World, UAudioComponent::StaticClass());
	if (IsValid(BGMComponentActive))
	{
		BGMComponentActive->bAutoDestroy = false;
		BGMComponentActive->SetupAttachment(nullptr);
		BGMComponentActive->RegisterComponentWithWorld(World);
	}

	BGMComponentStandby = NewObject<UAudioComponent>(World, UAudioComponent::StaticClass());
	if (IsValid(BGMComponentStandby))
	{
		BGMComponentStandby->bAutoDestroy = false;
		BGMComponentStandby->SetupAttachment(nullptr);
		BGMComponentStandby->RegisterComponentWithWorld(World);
	}

	bIsComponentAActive = true;

	ApplyBaseSoundMix();
}

void UHeistAudioSubsystem::ApplyBaseSoundMix()
{
	UWorld* World = GetWorld();
	if (IsValid(World) && IsValid(BaseSoundMix))
	{
		UGameplayStatics::SetBaseSoundMix(World, BaseSoundMix);
	}
}

void UHeistAudioSubsystem::SetBusVolume(EHeistAudioBus Bus, float NewVolume)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(BaseSoundMix)) return;

	USoundClass* TargetClass = GetSoundClassFromBus(Bus);
	if (!IsValid(TargetClass)) return;

	const float ClampedVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);

	UGameplayStatics::SetSoundMixClassOverride(World, BaseSoundMix, TargetClass, ClampedVolume, SoundMixPitchMultiplier, SoundMixVolumeFadeTime, true);

	switch (Bus)
	{
	case EHeistAudioBus::Master: MasterVolume = ClampedVolume; break;
	case EHeistAudioBus::BGM:    BGMVolume = ClampedVolume; break;
	case EHeistAudioBus::SFX:    SFXVolume = ClampedVolume; break;
	}
}

float UHeistAudioSubsystem::GetBusVolume(EHeistAudioBus Bus) const
{
	switch (Bus)
	{
	case EHeistAudioBus::Master: return MasterVolume;
	case EHeistAudioBus::BGM:    return BGMVolume;
	case EHeistAudioBus::SFX:    return SFXVolume;
	default: return 0.0f;
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

USoundClass* UHeistAudioSubsystem::GetSoundClassFromBus(EHeistAudioBus Bus) const
{
	switch (Bus)
	{
	case EHeistAudioBus::Master: return ClassMaster;
	case EHeistAudioBus::BGM:    return ClassBGM;
	case EHeistAudioBus::SFX:    return ClassSFX;
	default: return nullptr;
	}
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

	if (!IsValid(SoundData->SoundAsset)) return nullptr;

	UAudioComponent* FreeAudioComp = GetFreeAudioComponent();
	if (!IsValid(FreeAudioComp)) return nullptr;

	FreeAudioComp->SetSound(SoundData->SoundAsset);
	FreeAudioComp->AttenuationSettings = SoundData->AttenuationSettings;
	FreeAudioComp->SoundClassOverride = SoundData->SoundClass;

	FreeAudioComp->AttachToComponent(AttachToComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	FreeAudioComp->Play();

	return FreeAudioComp;


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

void UHeistAudioSubsystem::TransitionToBGM(EHeistSoundType NewBGMType, float OverrideFadeDuration, bool bStartFromBeginning)
{
	if (CurrentBGMType == NewBGMType) return;

	const FHeistSoundData* SoundData = GetSoundData(NewBGMType);
	if (SoundData == nullptr || !IsValid(SoundData->SoundAsset)) return;

	const float FadeDuration = (OverrideFadeDuration >= 0.0f) ? OverrideFadeDuration : SoundData->DefaultFadeDuration;

	UAudioComponent* ActiveBGM = bIsComponentAActive ? BGMComponentActive : BGMComponentStandby;
	UAudioComponent* NextBGM = bIsComponentAActive ? BGMComponentStandby : BGMComponentActive;

	if (!IsValid(ActiveBGM) || !IsValid(NextBGM)) return;

	// 1. 현재 재생 중인 BGM 페이드 아웃
	if (ActiveBGM->IsPlaying())
	{
		ActiveBGM->FadeOut(FadeDuration, 0.0f);
	}

	// 2. 새로운 BGM 세팅 및 페이드 인
	NextBGM->SetSound(SoundData->SoundAsset);
	NextBGM->SoundClassOverride = SoundData->SoundClass;

	const float StartTime = bStartFromBeginning ? 0.0f : -1.0f;
	NextBGM->FadeIn(FadeDuration, 1.0f, StartTime);

	// 3. 상태 스왑
	CurrentBGMType = NewBGMType;
	bIsComponentAActive = !bIsComponentAActive;
}
