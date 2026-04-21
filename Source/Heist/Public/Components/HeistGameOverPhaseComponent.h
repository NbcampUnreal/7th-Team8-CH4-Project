#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistGameOverPhaseComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HEIST_API UHeistGameOverPhaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHeistGameOverPhaseComponent();

	void StartEngineChanneling();

private:
	void EngineChannelingEnd();

	UPROPERTY(EditDefaultsOnly, Category = "Heist|GameOver")
	float EngineChannelingDuration = 20.f; // 기본 - 20초

	FTimerHandle EngineTimerHandle;
};
