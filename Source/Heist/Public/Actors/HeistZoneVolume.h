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
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	UPROPERTY(EditAnywhere, Category = "Heist|Zone")
	EHeistZoneType ZoneType = EHeistZoneType::Indoor;
};
