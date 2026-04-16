#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DropZoneVolume.generated.h"

class UBoxComponent;

UCLASS()
class HEIST_API ADropZoneVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	ADropZoneVolume();

	float GetValuePercent() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
public:	
	UPROPERTY(VisibleAnywhere, Category = "Heist|Volume")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "Heist|Volume")
	int32 TotalValue = 0;

	UPROPERTY(EditAnywhere, Category = "Heist|Volume")
	int32 TargetValue = 10;
};
