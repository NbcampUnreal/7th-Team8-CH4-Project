#pragma once

#include "CoreMinimal.h"
#include "Character/HeistCharacter.h"
#include "PoliceCharacter.generated.h"

class UFlashlightComponent;
/**
 * 경찰 전용 캐릭터.
 * 게임플레이 로직은 GA와 컴포넌트에 위임한다.
 *
 * TODO: 추후 추가 예정
 *   USoundDetectionComponent — 소리 탐지 핑
 */
UCLASS()
class HEIST_API APoliceCharacter : public AHeistCharacter
{
	GENERATED_BODY()

public:
	APoliceCharacter(const FObjectInitializer& ObjectInitializer);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Movement")
	float MaxRotationRateYaw = 200.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFlashlightComponent> FlashlightComponent;
};
