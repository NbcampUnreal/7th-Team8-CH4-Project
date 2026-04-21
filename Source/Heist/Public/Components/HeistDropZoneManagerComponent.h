#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistDropZoneManagerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HEIST_API UHeistDropZoneManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHeistDropZoneManagerComponent();

	void InitDropZone();

	// 구역 인덱스와 추가할 점수를 받음
	void UpdateZoneScore(int32 ZoneIndex, int32 CurrentScore);

public:
	UPROPERTY(VisibleAnywhere, Category = "Heist|Volume")
	int32 TargetScore = 25;
};
