#pragma once

#include "CoreMinimal.h"
#include "Character/HeistCharacter.h"
#include "PoliceCharacter.generated.h"

/**
 * 경찰 전용 캐릭터.
 * 게임플레이 로직은 GA와 컴포넌트에 위임한다.
 *
 * TODO: 추후 추가 예정
 *   UFlashlightComponent    — 손전등 Cone 판정, InFlashlight 태그 관리
 *   USoundDetectionComponent — 소리 탐지 핑
 */
UCLASS()
class HEIST_API APoliceCharacter : public AHeistCharacter
{
	GENERATED_BODY()

public:
	APoliceCharacter(const FObjectInitializer& ObjectInitializer);
};
