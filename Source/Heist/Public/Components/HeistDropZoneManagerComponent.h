#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistDropZoneManagerComponent.generated.h"

class ADropZoneVolume;
class AEscapeActor;

USTRUCT(BlueprintType)
struct FEscapeGroup
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ADropZoneVolume* DropZone;

	UPROPERTY(BlueprintReadOnly)
	AEscapeActor* EscapeActor;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HEIST_API UHeistDropZoneManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHeistDropZoneManagerComponent();

	void InitDropZone();

	// 구역 인덱스와 추가할 점수를 받음
	UFUNCTION()
	void UpdateZoneScore(int32 ZoneIndex, int32 CurrentScore);

	int32 GetZoneIndexByGroup(int32 GroupIndex);

public:
	UPROPERTY(VisibleAnywhere, Category = "Heist|Volume")
	int32 TargetScore = 20;

private:
	UPROPERTY()
	TMap<int32, FEscapeGroup> EscapeGroupMap;
};
