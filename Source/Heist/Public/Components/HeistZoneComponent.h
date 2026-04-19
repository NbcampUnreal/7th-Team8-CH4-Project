// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistZoneComponent.generated.h"

enum class EHeistZoneType : uint8;

UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UHeistZoneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeistZoneComponent();

	void UpdateZoneState(EHeistZoneType ZoneType, bool bIsEntering);

private:
	EHeistZoneType ResolveZoneFromCounts() const;
	void ApplyResolvedZone(EHeistZoneType NewResolvedZone);

	int32 IndoorVolumeCount = 0;
	int32 OutdoorVolumeCount = 0;
	EHeistZoneType ResolvedZone;
	bool bZoneInitialized = false;
};
