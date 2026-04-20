#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoundDetectionComponent.generated.h"

struct FGameplayEventData;

UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API USoundDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoundDetectionComponent();

	void ReceiveSoundDetection(const FVector& SoundLocation, float DetectionRadius);
protected:
	UFUNCTION(Client, Unreliable)
	void Client_ReceiveSoundDetection(const FVector& SoundLocation, float DetectionRadius);

private:
	void BroadcastToUI(const FVector& SoundLocation, float DetectionRadius);
};
