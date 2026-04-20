#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistLoadingScreenWidget.generated.h"

class UProgressBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadingFillComplete);

/**
 * 로딩 화면 위젯.
 *
 * 흐름:
 *   1. OnLoadingStarted(bIsSeamless) — 서브시스템이 위젯 표시 직후 호출.
 *      - bIsSeamless=true  : Progress Bar를 SeamlessIdleMaxProgress까지 서서히 채움.
 *      - bIsSeamless=false : 0%에서 정지 (스레드 블로킹 구간이므로 틱 없음).
 *   2. StartFillAnimation() — 맵 로드 완료 시 서브시스템이 호출.
 *      Progress Bar를 100%까지 채우고 OnFillComplete 브로드캐스트.
 *   3. OnFillComplete → 서브시스템이 위젯 숨김.
 *
 * Blueprint에서 "ProgressBar"라는 이름의 UProgressBar 위젯을 BindWidget으로 배치해야 함.
 */
UCLASS()
class HEISTUI_API UHeistLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void OnLoadingStarted_Implementation(bool bIsSeamless);
	virtual void StartFillAnimation_Implementation();

	UFUNCTION(BlueprintCallable)
	void OnLoadingStarted(bool bIsSeamless);

	UFUNCTION(BlueprintCallable)
	void StartFillAnimation();

	UPROPERTY(BlueprintAssignable)
	FOnLoadingFillComplete OnFillComplete;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar;

	// Seamless 구간에서 로딩 중 채울 최대 비율 (0 ~ 1)
	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	float SeamlessIdleMaxProgress = 0.85f;

	// Seamless 구간에서 로딩 중 채우는 속도 (초당 비율)
	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	float SeamlessFillSpeed = 0.15f;

	// StartFillAnimation 이후 100%까지 채우는 속도 (초당 비율)
	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	float CompletionFillSpeed = 1.5f;

private:
	float CurrentProgress = 0.0f;
	float TargetProgress = 0.0f;
	float CurrentFillSpeed = 0.0f;
	bool bFillCompleted = false;

	void SetProgressBarPercent(float Percent);
};
