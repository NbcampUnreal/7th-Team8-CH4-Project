#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlashlightComponent.generated.h"

class AThiefCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThiefSpotted, AThiefCharacter*, SpottedThief);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThiefLost, AThiefCharacter*, LostThief);

/**
 * 경찰 클라이언트 로컬에서 손전등 시야(Cone) 판정을 수행하는 컴포넌트.
 * 서버 동기화 없이 경찰 클라이언트가 직접 도둑 Actor의 가시성을 제어한다.
 *
 * 시야 규칙:
 *   Zone.Outdoor 태그 보유 도둑 — 항상 Visible
 *   Zone.Indoor  태그 보유 도둑 — Cone 내 진입 시에만 Visible
 *
 * 진입/이탈 시 OnThiefSpotted / OnThiefLost 델리게이트를 브로드캐스트한다.
 * HUD와 사운드는 이 델리게이트를 구독하여 처리한다.
 */
UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UFlashlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlashlightComponent();

	// HUD 및 사운드에서 구독
	UPROPERTY(BlueprintAssignable, Category = "Heist|Vision")
	FOnThiefSpotted OnThiefSpotted;

	UPROPERTY(BlueprintAssignable, Category = "Heist|Vision")
	FOnThiefLost OnThiefLost;

protected:
	virtual void BeginPlay() override;


private:
	// TODO(하민): Data 에셋이나 테이블에서 관리하도록 추후 연결
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Vision")
	float FlashlightRadius = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Vision")
	float FlashlightHalfAngle = 30.0f;

	UPROPERTY()
	TSet<AThiefCharacter*> PreviouslyVisibleThieves;

	FTimerHandle VisionCheckTimerHandle;

	UFUNCTION()
	void ProcessLocalVision();

	bool IsThiefInFlashlight(AThiefCharacter* Thief, bool bWasPreviouslyVisible) const;
};
