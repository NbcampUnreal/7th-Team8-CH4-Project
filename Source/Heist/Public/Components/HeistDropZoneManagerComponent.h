#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistDropZoneManagerComponent.generated.h"

class ADropZoneVolume;
class AEscapeActor;
class AVehicleActor;

USTRUCT(BlueprintType)
struct FEscapeGroup
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ADropZoneVolume* DropZone;

	UPROPERTY(BlueprintReadOnly)
	AEscapeActor* EscapeActor;

	UPROPERTY(BlueprintReadOnly)
	AVehicleActor* VehicleActor;
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

	bool CheckDoorMoving(int32 GroupIndex);
	bool CheckDoorOpened(int32 GroupIndex);
	void CloseDoor(int32 GroupIndex);
	void OpenDoor(int32 GroupIndex);

public:
	UPROPERTY(VisibleAnywhere, Category = "Heist|Volume")
	int32 TargetScore = 20;

private:
	UPROPERTY()
	TMap<int32, FEscapeGroup> EscapeGroupMap;
};
