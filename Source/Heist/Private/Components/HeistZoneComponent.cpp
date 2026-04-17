#include "Components/HeistZoneComponent.h"

#include "Character/HeistTags_State.h"
#include "Actors/HeistZoneVolume.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

UHeistZoneComponent::UHeistZoneComponent()
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

	// 2. 태그 갱신
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	if (!IsValid(ASC)) return;

	// 변경점: AddLooseGameplayTag -> AddReplicatedLooseGameplayTag 로 수정
	if (OutdoorVolumeCount > 0)
	{
		ASC->AddReplicatedLooseGameplayTag(HeistStateTags::Zone_Outdoor);
		ASC->RemoveReplicatedLooseGameplayTag(HeistStateTags::Zone_Indoor);
	}
	else if (IndoorVolumeCount > 0)
	{
		ASC->AddReplicatedLooseGameplayTag(HeistStateTags::Zone_Indoor);
		ASC->RemoveReplicatedLooseGameplayTag(HeistStateTags::Zone_Outdoor);
	}
	else
	{
		ASC->AddReplicatedLooseGameplayTag(HeistStateTags::Zone_Outdoor);
		ASC->RemoveReplicatedLooseGameplayTag(HeistStateTags::Zone_Indoor);
	}
}
