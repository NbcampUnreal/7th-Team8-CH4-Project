#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "HeistZoneVolume.generated.h"

UENUM(BlueprintType)
enum class EHeistZoneType : uint8
{
	Indoor,
	Outdoor
};

UCLASS()
class HEIST_API AHeistZoneVolume : public AVolume
{
	GENERATED_BODY()

public:
	AHeistZoneVolume();

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

private:
	UPROPERTY(EditAnywhere, Category = "Heist|Zone")
	EHeistZoneType ZoneType = EHeistZoneType::Indoor;
};
