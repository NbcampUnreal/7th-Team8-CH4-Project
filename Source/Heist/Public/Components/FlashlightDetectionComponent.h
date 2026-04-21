#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlashlightDetectionComponent.generated.h"


UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UFlashlightDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlashlightDetectionComponent();

	void StartDetection();
	void StopDetection();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Detection")
	float DetectionCheckInterval = 0.1f;

	FTimerHandle AlertCheckTimerHandle;

	bool bWasInFlashlight;

	UFUNCTION()
	void ProcessDetectionCheck();
};
