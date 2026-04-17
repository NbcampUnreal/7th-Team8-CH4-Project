#include "Actors/HeistZoneVolume.h"

#include "Components/HeistZoneComponent.h"

#include "EngineUtils.h"

AHeistZoneVolume::AHeistZoneVolume()
{
	bGenerateOverlapEventsDuringLevelStreaming = true;
}

void AHeistZoneVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!HasAuthority() || !IsValid(OtherActor)) return;

	UHeistZoneComponent* ZoneComp = OtherActor->FindComponentByClass<UHeistZoneComponent>();
	if (IsValid(ZoneComp))
	{
		ZoneComp->UpdateZoneState(ZoneType, true);
	}
}

void AHeistZoneVolume::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (!HasAuthority() || !IsValid(OtherActor)) return;

	UHeistZoneComponent* ZoneComp = OtherActor->FindComponentByClass<UHeistZoneComponent>();
	if (IsValid(ZoneComp))
	{
		ZoneComp->UpdateZoneState(ZoneType, false);
	}
}
