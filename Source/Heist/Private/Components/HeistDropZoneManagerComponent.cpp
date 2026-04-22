#include "Components/HeistDropZoneManagerComponent.h"
#include "Core/HeistMatchGameMode.h"
#include "Core/HeistMatchGameState.h"
#include "Actors/DropZoneVolume.h"
#include "Actors/EscapeActor.h"
#include "Actors/VehicleActor.h"
#include "EngineUtils.h"

UHeistDropZoneManagerComponent::UHeistDropZoneManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UHeistDropZoneManagerComponent::InitDropZone()
{
	if (!GetOwner()->HasAuthority()) return;

	AHeistMatchGameMode* HeistGM = GetOwner<AHeistMatchGameMode>();
	if (!IsValid(HeistGM)) return;

	AHeistMatchGameState* HeistGS = HeistGM->GetGameState<AHeistMatchGameState>();
	if (!HeistGS) return;

	TMap<int32, AEscapeActor*> TempEscapeMap;
	for (TActorIterator<AEscapeActor> It(GetWorld()); It; ++It)
	{
		if (AEscapeActor* Actor = *It)
		{
			// 에디터에서 설정한 GroupIndex를 키로 저장
			TempEscapeMap.Add(Actor->EscapeGroupIndex, Actor);
		}
	}

	TMap<int32, AVehicleActor*> TempVehicleMap;
	for (TActorIterator<AVehicleActor> It(GetWorld()); It; ++It)
	{
		if (AVehicleActor* Vehicle = *It)
		{
			TempVehicleMap.Add(Vehicle->EscapeGroupIndex, Vehicle);
		}
	}

	TArray<ADropZoneVolume*> DropZones;
	for (TActorIterator<ADropZoneVolume> It(GetWorld()); It; ++It)
	{
		if (ADropZoneVolume* Zone = *It)
		{
			DropZones.Add(Zone);
		}
	}

	DropZones.Sort([](const ADropZoneVolume& A, const ADropZoneVolume& B)
	{
		return A.EscapeGroupIndex < B.EscapeGroupIndex;
	});

	int32 ZoneVolumeCount = 0;
	for (ADropZoneVolume* Zone : DropZones)
	{
		Zone->ZoneIndex = ZoneVolumeCount;
		Zone->OnZoneValueChanged.AddDynamic(this, &UHeistDropZoneManagerComponent::UpdateZoneScore);

		FEscapeGroup& NewGroup = EscapeGroupMap.FindOrAdd(ZoneVolumeCount);
		NewGroup.DropZone = Zone;

		if (AEscapeActor** FoundActor = TempEscapeMap.Find(Zone->EscapeGroupIndex))
		{
			NewGroup.EscapeActor = *FoundActor;
		}

		if (AVehicleActor** FoundVehicle = TempVehicleMap.Find(Zone->EscapeGroupIndex))
		{
			NewGroup.VehicleActor = *FoundVehicle;
		}

		ZoneVolumeCount++;
	}

	HeistGS->InitZoneScores(ZoneVolumeCount, TargetScore);
}

void UHeistDropZoneManagerComponent::UpdateZoneScore(int32 ZoneIndex, int32 CurrentScore)
{
	// 1. GameState 가져오기
	AHeistMatchGameMode* HeistGM = GetOwner<AHeistMatchGameMode>();
	if (!IsValid(HeistGM)) return;

	AHeistMatchGameState* HeistGS = HeistGM->GetGameState<AHeistMatchGameState>();
	if (!HeistGS) return;

	// 2. GameState의 데이터 수정 (서버에서 수정하면 클라이언트로 복제됨)
	HeistGS->SetZoneScore(ZoneIndex, CurrentScore); 

	// 3. 승리 조건 체크
	if (HeistGS->GetZoneScore(ZoneIndex).CurrentScore >= HeistGS->GetZoneScore(ZoneIndex).TargetScore)
	{
		UE_LOG(LogTemp, Warning, TEXT("%d번 구역 목표 달성!"), ZoneIndex);
		if (FEscapeGroup* Group = EscapeGroupMap.Find(ZoneIndex))
		{
			if (Group->EscapeActor)
			{
				Group->EscapeActor->SetActivation(true);
			}
		}
	}
	else
	{
		if (FEscapeGroup* Group = EscapeGroupMap.Find(ZoneIndex))
		{
			if (Group->EscapeActor)
			{
				Group->EscapeActor->SetActivation(false);
			}
		}
	}
}

int32 UHeistDropZoneManagerComponent::GetZoneIndexByGroup(int32 GroupIndex)
{
	for (auto& Elem : EscapeGroupMap)
	{
		if (Elem.Value.EscapeActor->EscapeGroupIndex == GroupIndex)
		{
			return Elem.Key;
		}
	}
	return 0;
}

bool UHeistDropZoneManagerComponent::CheckDoorMoving(int32 GroupIndex)
{
	for (auto& Elem : EscapeGroupMap)
	{
		if (Elem.Value.VehicleActor && Elem.Value.VehicleActor->EscapeGroupIndex == GroupIndex)
		{
			return Elem.Value.VehicleActor->GetDoorMoving();
		}
	}
	return false;
}

bool UHeistDropZoneManagerComponent::CheckDoorOpened(int32 GroupIndex)
{
	for (auto& Elem : EscapeGroupMap)
	{
		if (Elem.Value.VehicleActor && Elem.Value.VehicleActor->EscapeGroupIndex == GroupIndex)
		{
			return Elem.Value.VehicleActor->GetDoorOpened();
		}
	}
	return false;
}

void UHeistDropZoneManagerComponent::CloseDoor(int32 GroupIndex)
{
	for (auto& Elem : EscapeGroupMap)
	{
		if (Elem.Value.VehicleActor && Elem.Value.VehicleActor->EscapeGroupIndex == GroupIndex)
		{
			Elem.Value.VehicleActor->CloseDoor();
			Elem.Value.VehicleActor->SetDoorOpened(false);
			break;
		}
	}
}

void UHeistDropZoneManagerComponent::OpenDoor(int32 GroupIndex)
{
	for (auto& Elem : EscapeGroupMap)
	{
		if (Elem.Value.VehicleActor && Elem.Value.VehicleActor->EscapeGroupIndex == GroupIndex)
		{
			Elem.Value.VehicleActor->OpenDoor();
			Elem.Value.VehicleActor->SetDoorOpened(true);
			break;
		}
	}
}
