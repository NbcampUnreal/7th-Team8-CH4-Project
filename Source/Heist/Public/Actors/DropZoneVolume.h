#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DropZoneVolume.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneValueChanged, int32, ZoneIndex, int32, CurrentValue);

UCLASS()
class HEIST_API ADropZoneVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	ADropZoneVolume();

	float GetCurrentValue() const { return CurrentValue; }

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:	
	UPROPERTY(VisibleAnywhere, Category = "Heist|Volume")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Heist|Volume")
	int32 CurrentValue;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Heist|Volume")
	int32 ZoneIndex;

	UPROPERTY(EditAnywhere, Category = "Heist|Escape")
	int32 EscapeGroupIndex = 0;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnZoneValueChanged OnZoneValueChanged;
};
