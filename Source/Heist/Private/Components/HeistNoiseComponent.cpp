#include "Components/HeistNoiseComponent.h"

#include "Data/HeistSoundData.h"
#include "Character/PoliceCharacter.h"
#include "Character/HeistTags_State.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "AbilitySystem/HeistTags_FlagTags.h"
#include "Components/SoundDetectionComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UHeistNoiseComponent::UHeistNoiseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHeistNoiseComponent::StartChannelingNoise(EHeistSoundType SoundType)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	if (OwnerPawn->HasAuthority() || OwnerPawn->IsLocallyControlled())
	{
		Server_StartChannelingNoise(SoundType);
	}
}

void UHeistNoiseComponent::StopChannelingNoise()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	if (OwnerPawn->HasAuthority() || OwnerPawn->IsLocallyControlled())
	{
		Server_StopChannelingNoise();
	}
}

void UHeistNoiseComponent::Server_StartChannelingNoise_Implementation(EHeistSoundType SoundType)
{
	const FHeistSoundData* SoundData = GetSoundData(SoundType);
	if (SoundData == nullptr) return;

	if (SoundData->bAffectsPoliceAlert || SoundData->bInstantAlert)
	{
		UAbilitySystemComponent* OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
		const float FinalRadius = CalculateFinalDetectionRadius(SoundData, OwnerASC);
		DetectPoliceAndSendEvent(SoundData, FinalRadius, GetOwner()->GetActorLocation());
	}
}

void UHeistNoiseComponent::Server_StopChannelingNoise_Implementation()
{
	// TODO (하민): 채널링 중단 시 경찰 AI의 추가적인 상태 초기화가 필요하다면 여기 작성
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
	if (SoundType == EHeistSoundType::Footstep_Thief)
	{
		UWorld* World = GetWorld();
		if (!IsValid(World)) return;

		const float CurrentTime = World->GetTimeSeconds();
		if (CurrentTime - LastFootstepTime < FootstepDebounceTime)
		{
			return; // 쿨타임 미달 시 무시 (탐지/사운드 연산 스킵)
		}
		LastFootstepTime = CurrentTime;
	}

	const FHeistSoundData* SoundData = GetSoundData(SoundType);
	if (SoundData == nullptr) return;

	if (SoundData->bAffectsPoliceAlert || SoundData->bInstantAlert)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
		float FinalRadius = CalculateFinalDetectionRadius(SoundData, ASC);

		DetectPoliceAndSendEvent(SoundData, FinalRadius, OriginLocation);
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

	float BaseRadius = SoundData->BaseRadius;
	float FinalRadius = BaseRadius;

	if (!IsValid(OwnerASC)) return FinalRadius;

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
			USoundDetectionComponent* DetectionComp = PoliceCharacter->GetComponentByClass<USoundDetectionComponent>();
			if (IsValid(DetectionComp))
			{
				DetectionComp->ReceiveSoundDetection(OriginLocation, FinalRadius);
			}
		}
	}
}

