#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestNoiseActor.generated.h"

UCLASS()
class HEIST_API ATestNoiseActor : public AActor
{
	GENERATED_BODY()

public:
	ATestNoiseActor();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void FireTestNoise();

	FTimerHandle TestTimerHandle;
};
