#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoliceCar_Trigger.generated.h"

class UBoxComponent;

UCLASS()
class HEIST_API APoliceCar_Trigger : public AActor
{
	GENERATED_BODY()

public:
	APoliceCar_Trigger();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere, Category = "Heist|Trigger")
	TObjectPtr<UBoxComponent> TriggerBox;
};
