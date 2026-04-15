
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/HeistBriefingWidgetInterface.h"
#include "HeistBriefingScreenWidget.generated.h"

class UButton;
class UCanvasPanel;
class UHeistBriefingMapWidget;
class UHeistBriefingPointDataAsset;
class UTextBlock;

UCLASS()
class HEISTUI_API UHeistBriefingPointButtonBinding : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(class UHeistBriefingScreenWidget* InOwner, const FHeistSpawnPointData& InPointData);

	UFUNCTION()
	void HandleClicked();

private:
	UPROPERTY()
	TObjectPtr<class UHeistBriefingScreenWidget> Owner;

	FHeistSpawnPointData PointData;
};

/**
 * 브리핑 전체 화면 래퍼 위젯.
 *
 * 이 위젯은 좌/우 패널, 버튼, 오버레이 등 브리핑 화면 레이아웃의 최상위 컨테이너이며
 * 실제 맵 드로잉 입력/렌더는 내부의 UHeistBriefingMapWidget이 담당한다.
 *
 * PlayerController는 이 위젯만 생성하고 IHeistBriefingWidgetInterface를 통해 초기화한다.
 * 이후 이 위젯이 내부 MapWidget으로 브리핑 컨텍스트를 전달한다.
 */
UCLASS()
class HEISTUI_API UHeistBriefingScreenWidget : public UUserWidget, public IHeistBriefingWidgetInterface
{
	GENERATED_BODY()

public:
	// IHeistBriefingWidgetInterface
	virtual void InitializeForBriefing(
		UHeistBriefingPlayerComponent* InBriefingPlayerComponent,
		UHeistBriefingDrawingSyncComponent* InDrawingSyncComponent,
		EHeistBriefingViewMode InViewMode) override;

	/** BP 버튼 위젯이 호출하는 선택 진입점. 포인트 타입에 따라 적절한 서버 RPC로 라우팅한다. */
	UFUNCTION(BlueprintCallable, Category = "Heist|Briefing")
	void SelectBriefingPointByData(const FHeistSpawnPointData& InPointData);

	/** 현재 팀/레이어 기준으로 노출 가능한 포인트 목록을 반환한다. */
	UFUNCTION(BlueprintCallable, Category = "Heist|Briefing")
	void GetVisibleBriefingPoints(TArray<FHeistSpawnPointData>& OutVisiblePoints) const;

	/** BP에서 레이어 변경 후 버튼 목록을 다시 만들고 싶을 때 호출한다. */
	UFUNCTION(BlueprintCallable, Category = "Heist|Briefing")
	void RefreshBriefingPointButtons();

	/** 버튼 위젯 내부 표시에 추가 커스터마이징이 필요하면 BP에서 보정한다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Heist|Briefing")
	void ConfigureBriefingPointButton(UUserWidget* ButtonWidget, const FHeistSpawnPointData& PointData);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * WBP_BriefingScreen 내부에 배치되는 맵 전용 서브위젯.
	 * 변수 이름을 반드시 "MapWidget"으로 맞춰 BindWidget 되도록 설정한다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHeistBriefingMapWidget> MapWidget;

	/**
	 * 브리핑 포인트 정의 테이블. GameMode BP에서 쓰는 것과 동일한 DA 인스턴스를 할당한다.
	 * UI는 NormalizedPosition / DisplayName / VisibleTo 를 읽어 버튼을 배치한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heist|Briefing")
	TObjectPtr<UHeistBriefingPointDataAsset> BriefingPointData;

	/**
	 * 포인트 버튼이 배치될 Canvas.
	 * WBP_BriefingScreen에서 변수 이름을 반드시 "PointButtonCanvas"로 맞춘다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> PointButtonCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Timer;

	/**
	 * 포인트 버튼 위젯 클래스.
	 * 내부에 "Button_Select"(UButton), "Text_DisplayName"(UTextBlock) 위젯 이름을 맞추면
	 * 코드에서 클릭/라벨을 자동 연결한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heist|Briefing")
	TSubclassOf<UUserWidget> SpawnPointButtonClass;

private:
	friend class UHeistBriefingPointButtonBinding;

	UFUNCTION()
	void HandleMapInitialized(UHeistBriefingPlayerComponent* InBriefingPlayerComponent);

	void NotifyVisiblePointsChanged();
	void RebuildBriefingPointButtonsInternal(const TArray<FHeistSpawnPointData>& VisiblePoints);
	void ClearBriefingPointButtons();
	void HandleBriefingPointButtonClicked(const FHeistSpawnPointData& PointData);

	EHeistBriefingViewMode CurrentViewMode = EHeistBriefingViewMode::Thief;
	bool bMapInitialized = false;

	UPROPERTY(Transient)
	TObjectPtr<UHeistBriefingPlayerComponent> CachedBriefingPlayerComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> SpawnedPointButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHeistBriefingPointButtonBinding>> PointButtonBindings;
};
