#include "Actors/HeistZoneVolume.h"

#include "Components/HeistZoneComponent.h"

#include "Components/BrushComponent.h"
#include "EngineUtils.h"

AHeistZoneVolume::AHeistZoneVolume()
{
	bGenerateOverlapEventsDuringLevelStreaming = true;
}

void AHeistZoneVolume::BeginPlay()
{
	Super::BeginPlay();

	if (UBrushComponent* BrushComp = GetBrushComponent())
	{
		BrushComp->OnComponentBeginOverlap.AddDynamic(this, &AHeistZoneVolume::OnZoneBeginOverlap);
		BrushComp->OnComponentEndOverlap.AddDynamic(this, &AHeistZoneVolume::OnZoneEndOverlap);
	}

	if (HasAuthority())
	{
		TArray<AActor*> OverlappingActors;
		GetOverlappingActors(OverlappingActors);

		for (AActor* Actor : OverlappingActors)
		{
			if (!IsValid(Actor)) continue;

			UHeistZoneComponent* ZoneComp = Actor->FindComponentByClass<UHeistZoneComponent>();
			if (IsValid(ZoneComp))
			{
				ZoneComp->UpdateZoneState(ZoneType, true);
			}
		}
	}
}

void AHeistZoneVolume::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValid(OtherActor) || !IsValid(OtherComp)) return;

	if (OtherComp != OtherActor->GetRootComponent()) return;

	UHeistZoneComponent* ZoneComp = OtherActor->FindComponentByClass<UHeistZoneComponent>();
	if (IsValid(ZoneComp))
	{
		ZoneComp->UpdateZoneState(ZoneType, true);
	}
}

void AHeistZoneVolume::OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || !IsValid(OtherActor) || !IsValid(OtherComp)) return;

	if (OtherComp != OtherActor->GetRootComponent()) return;

	UHeistZoneComponent* ZoneComp = OtherActor->FindComponentByClass<UHeistZoneComponent>();
	if (IsValid(ZoneComp))
	{
		ZoneComp->UpdateZoneState(ZoneType, false);
	}
}
