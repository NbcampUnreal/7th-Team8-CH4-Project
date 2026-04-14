#include "Components/HeistNoiseComponent.h"

#include "Data/HeistSoundData.h"
#include "Character/PoliceCharacter.h"
#include "Character/HeistTags_State.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "AbilitySystem/HeistTags_FlagTags.h"
#include "Components/AudioComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

UHeistNoiseComponent::UHeistNoiseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHeistNoiseComponent::StartChannelingNoise(EHeistSoundType SoundType)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	if (OwnerPawn->HasAuthority())
	{
		Server_StartChannelingNoise_Implementation(SoundType);
	}
	else if (OwnerPawn->IsLocallyControlled())
	{
		Server_StartChannelingNoise(SoundType);
	}
}

void UHeistNoiseComponent::StopChannelingNoise()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	if (OwnerPawn->HasAuthority())
	{
		Server_StopChannelingNoise_Implementation();
	}
	else if (OwnerPawn->IsLocallyControlled())
	{
		Server_StopChannelingNoise();
	}
}

void UHeistNoiseComponent::Server_StartChannelingNoise_Implementation(EHeistSoundType SoundType)
{
	const FHeistSoundData* SoundData = GetSoundData(SoundType);
	if (SoundData == nullptr) return;

	// 1. 모든 클라이언트에서 루프 오디오 재생
	Multicast_StartChannelingNoise(SoundType);

	// 2. 경찰 탐지 핑
	if (SoundData->bAffectsPoliceAlert || SoundData->bInstantAlert)
	{
		UAbilitySystemComponent* OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
		const float FinalRadius = CalculateFinalDetectionRadius(SoundData, OwnerASC);
		DetectPoliceAndSendEvent(SoundData, FinalRadius, GetOwner()->GetActorLocation());
	}
}

void UHeistNoiseComponent::Multicast_StartChannelingNoise_Implementation(EHeistSoundType SoundType)
{
	const FHeistSoundData* SoundData = GetSoundData(SoundType);
	if (SoundData == nullptr) return;

	if (IsValid(ActiveChannelingAudio))
	{
		ActiveChannelingAudio->Stop();
		ActiveChannelingAudio->DestroyComponent();
		ActiveChannelingAudio = nullptr;
	}

	if (IsValid(SoundData->SoundAsset))
	{
		constexpr float DefaultVolumeMultiplier = 1.0f;
		constexpr float DefaultPitchMultiplier = 1.0f;
		constexpr float DefaultStartTime = 0.0f;

		ActiveChannelingAudio = UGameplayStatics::SpawnSoundAttached(
			SoundData->SoundAsset,
			GetOwner()->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget,
			false, // bAutoDestroy
			DefaultVolumeMultiplier,
			DefaultPitchMultiplier,
			DefaultStartTime,
			SoundData->AttenuationSettings
		);
	}
}

void UHeistNoiseComponent::Server_StopChannelingNoise_Implementation()
{
	Multicast_StopChannelingNoise();
}

void UHeistNoiseComponent::Multicast_StopChannelingNoise_Implementation()
{
	if (IsValid(ActiveChannelingAudio))
	{
		constexpr float FadeOutDuration = 0.2f;
		constexpr float FadeVolumeLevel = 0.0f;

		ActiveChannelingAudio->FadeOut(FadeOutDuration, FadeVolumeLevel);
		ActiveChannelingAudio = nullptr;
	}
}

void UHeistNoiseComponent::MakeHeistNoise(EHeistSoundType SoundType, FVector OriginLocation)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	if (OwnerPawn->HasAuthority() || OwnerPawn->IsLocallyControlled())
	{
		Server_ProcessNoise(SoundType, OriginLocation);
	}
}

void UHeistNoiseComponent::Server_ProcessNoise_Implementation(EHeistSoundType SoundType, FVector OriginLocation)
{
	const FHeistSoundData* SoundData = GetSoundData(SoundType);
	if (SoundData == nullptr) return;

	// 1. 오디오는 모든 클라이언트가 들을 수 있도록 Multicast 호출
	Multicast_PlaySound(SoundType, OriginLocation);

	// 2. 경찰 탐지 처리 (서버에서만 연산)
	if (SoundData->bAffectsPoliceAlert || SoundData->bInstantAlert)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
		float FinalRadius = CalculateFinalDetectionRadius(SoundData, ASC);

		DetectPoliceAndSendEvent(SoundData, FinalRadius, OriginLocation);
	}
}

void UHeistNoiseComponent::Multicast_PlaySound_Implementation(EHeistSoundType SoundType, FVector OriginLocation)
{
	const FHeistSoundData* SoundData = GetSoundData(SoundType);
	if (SoundData == nullptr) return;

	if (IsValid(SoundData->SoundAsset))
	{
		constexpr float DefaultVolumeMultiplier = 1.0f;
		constexpr float DefaultPitchMultiplier = 1.0f;
		constexpr float DefaultStartTime = 0.0f;

		UGameplayStatics::PlaySoundAtLocation(
			this,
			SoundData->SoundAsset,
			OriginLocation,
			DefaultVolumeMultiplier,
			DefaultPitchMultiplier,
			DefaultStartTime,
			SoundData->AttenuationSettings
		);
	}
}

const FHeistSoundData* UHeistNoiseComponent::GetSoundData(EHeistSoundType SoundType) const
{
	if (!IsValid(SoundDataTable)) return nullptr;

	static const FString ContextString(TEXT("HeistNoiseComponent::GetSoundData"));

	const UEnum* EnumPtr = StaticEnum<EHeistSoundType>();
	if (!IsValid(EnumPtr)) return nullptr;

	const FName RowName = FName(*EnumPtr->GetNameStringByValue(static_cast<int64>(SoundType)));
	return SoundDataTable->FindRow<FHeistSoundData>(RowName, ContextString);
}

float UHeistNoiseComponent::CalculateFinalDetectionRadius(const FHeistSoundData* SoundData, UAbilitySystemComponent* OwnerASC) const
{
	if (SoundData == nullptr) return 0.0f;
	if (!IsValid(OwnerASC)) return SoundData->BaseRadius;

	float FinalRadius = SoundData->BaseRadius;

	// 부상 상태 체크
	if (OwnerASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured))
	{
		FinalRadius *= SoundData->InjuredRadiusMultiplier;
	}

	// 물건 운반 상태 체크
	if (OwnerASC->HasMatchingGameplayTag(HeistFlagTags::Tag_Carrying))
	{
		FinalRadius *= SoundData->CarryingRadiusMultiplier;
	}

	return FinalRadius;
}

void UHeistNoiseComponent::DetectPoliceAndSendEvent(const FHeistSoundData* SoundData, float FinalRadius, FVector OriginLocation)
{
	if (SoundData == nullptr) return;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	const float RadiusSquared = FinalRadius * FinalRadius;

	for (TActorIterator<APoliceCharacter> Iterator(World); Iterator; ++Iterator)
	{
		APoliceCharacter* PoliceCharacter = *Iterator;
		if (!IsValid(PoliceCharacter)) continue;

		bool bShouldAlert = SoundData->bInstantAlert;

		if (!bShouldAlert)
		{
			const float DistanceSquared = FVector::DistSquared(OriginLocation, PoliceCharacter->GetActorLocation());
			bShouldAlert = (DistanceSquared <= RadiusSquared);
		}

		if (bShouldAlert)
		{
			FGameplayEventData Payload;
			Payload.Instigator = GetOwner();
			Payload.Target = PoliceCharacter;
			Payload.EventMagnitude = FinalRadius;

			// TODO(하민): UI 방향 핑(Ping) 처리를 위해 오디오 원점(OriginLocation)을 
			// TargetData나 ContextHandle 등에 담아 Payload로 넘기는 추가 작업 예정

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				PoliceCharacter,
				HeistEventTags::Event_SoundDetected,
				Payload
			);
		}
	}
}