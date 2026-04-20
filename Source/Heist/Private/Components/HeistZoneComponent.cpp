#include "Components/HeistZoneComponent.h"

#include "Character/HeistTags_State.h"
#include "Actors/HeistZoneVolume.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

UHeistZoneComponent::UHeistZoneComponent()
	: ResolvedZone(EHeistZoneType::Outdoor)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHeistZoneComponent::UpdateZoneState(EHeistZoneType ZoneType, bool bIsEntering)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()) return;

	// 1. 카운트 갱신
	if (ZoneType == EHeistZoneType::Indoor)
	{
		IndoorVolumeCount += bIsEntering ? 1 : -1;
		IndoorVolumeCount = FMath::Max(0, IndoorVolumeCount);
	}
	else
	{
		OutdoorVolumeCount += bIsEntering ? 1 : -1;
		OutdoorVolumeCount = FMath::Max(0, OutdoorVolumeCount);
	}

	const EHeistZoneType NewResolvedZone = ResolveZoneFromCounts();
	if (bZoneInitialized && ResolvedZone == NewResolvedZone)
	{
		return;
	}

	ApplyResolvedZone(NewResolvedZone);
}

EHeistZoneType UHeistZoneComponent::ResolveZoneFromCounts() const
{
	if (OutdoorVolumeCount > 0)
	{
		return EHeistZoneType::Outdoor;
	}

	if (IndoorVolumeCount > 0)
	{
		return EHeistZoneType::Indoor;
	}

	return EHeistZoneType::Outdoor;
}

void UHeistZoneComponent::ApplyResolvedZone(EHeistZoneType NewResolvedZone)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	if (!IsValid(ASC)) return;

	if (bZoneInitialized)
	{
		if (ResolvedZone == EHeistZoneType::Indoor)
		{
			ASC->RemoveReplicatedLooseGameplayTag(HeistStateTags::Zone_Indoor);
		}
		else
		{
			ASC->RemoveReplicatedLooseGameplayTag(HeistStateTags::Zone_Outdoor);
		}
	}

	if (NewResolvedZone == EHeistZoneType::Indoor)
	{
		ASC->AddReplicatedLooseGameplayTag(HeistStateTags::Zone_Indoor);
	}
	else
	{
		ASC->AddReplicatedLooseGameplayTag(HeistStateTags::Zone_Outdoor);
	}

	ResolvedZone = NewResolvedZone;
	bZoneInitialized = true;
}
