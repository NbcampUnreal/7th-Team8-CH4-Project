
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/HeistMatchTypes.h"
#include "HeistBriefingPointDataAsset.generated.h"

/**
 * 브리핑 포인트 정의 테이블.
 *
 * 서버(PhaseComponent)와 클라이언트(ScreenWidget) 양쪽이 이 에셋 하나를 참조한다.
 * 서버는 WorldTag로 레벨 액터를 수집하고, UI는 NormalizedPosition으로 버튼을 배치한다.
 *
 * GameMode BP와 WBP_BriefingScreen 양쪽에서 같은 DA 인스턴스를 할당할 것.
 */
UCLASS()
class HEIST_API UHeistBriefingPointDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heist|Briefing")
	TArray<FHeistSpawnPointData> Points;
};
