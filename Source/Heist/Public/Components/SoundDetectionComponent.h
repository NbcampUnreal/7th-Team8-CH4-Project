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

protected:
	virtual void BeginPlay() override;

private:
	void OnSoundDetectedEvent(const FGameplayEventData* Payload);
};
