#include "Components/HeistDropZoneManagerComponent.h"
#include "Core/HeistMatchGameMode.h"
#include "Core/HeistMatchGameState.h"
#include "Actors/DropZoneVolume.h"
#include "Actors/EscapeActor.h"
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

	int32 ZoneVolumeCount = 0;
	for (TActorIterator<ADropZoneVolume> It(GetWorld()); It; ++It)
	{
		ADropZoneVolume* Zone = *It;
		if (Zone)
		{
			Zone->ZoneIndex = ZoneVolumeCount;
			Zone->OnZoneValueChanged.AddDynamic(this, &UHeistDropZoneManagerComponent::UpdateZoneScore);

			FEscapeGroup& NewGroup = EscapeGroupMap.FindOrAdd(ZoneVolumeCount);
			NewGroup.DropZone = Zone;

			// 해당 Zone의 GroupIndex와 일치하는 EscapeActor가 TempMap에 있는지 확인
			if (AEscapeActor** FoundActor = TempEscapeMap.Find(Zone->EscapeGroupIndex))
			{
				NewGroup.EscapeActor = *FoundActor;
			}

			ZoneVolumeCount++;
		}
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
