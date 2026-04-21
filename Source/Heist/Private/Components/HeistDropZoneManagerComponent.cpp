#include "Components/HeistDropZoneManagerComponent.h"
#include "Core/HeistMatchGameMode.h"
#include "Core/HeistMatchGameState.h"
#include "Actors/DropZoneVolume.h"
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

	int32 ZoneVolumeCount = 0;
	for (TActorIterator<ADropZoneVolume> It(GetWorld()); It; ++It)
	{
		ADropZoneVolume* Zone = *It;
		if (Zone)
		{
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
	}
}
