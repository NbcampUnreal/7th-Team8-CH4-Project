
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Core/HeistMatchTypes.h"
#include "HeistBriefingWidgetInterface.h"
#include "HeistBriefingMapWidget.generated.h"

class UHeistBriefingDrawingSyncComponent;
class UHeistBriefingPlayerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBriefingMapInitialized, UHeistBriefingPlayerComponent*, BriefingPlayerComponent);
struct FGeometry;
struct FPointerEvent;
class FPaintArgs;
class FSlateRect;
class FSlateWindowElementList;
class FWidgetStyle;

/**
 * 브리핑 맵 위에 표시되는 UMG 위젯.
 *
 * IHeistBriefingWidget을 구현해 PlayerController로부터 직접 초기화 호출을 받는다.
 * 맵/플랜 표시, 브리핑 드로잉 입력, committed stroke 렌더,
 * preview chunk 반영을 담당한다.
 */
UCLASS()
class HEISTUI_API UHeistBriefingMapWidget : public UUserWidget, public IHeistBriefingWidgetInterface
{
	GENERATED_BODY()

public:
	// IHeistBriefingWidget
	virtual void InitializeForBriefing(
		UHeistBriefingPlayerComponent* InBriefingPlayerComponent,
		UHeistBriefingDrawingSyncComponent* InDrawingSyncComponent,
		EHeistBriefingViewMode InViewMode) override;

	UFUNCTION(BlueprintCallable, Category = "Heist|Briefing")
	void SetCurrentDrawingTool(EHeistBriefingDrawingTool InTool);

	/** WidgetSwitcher와 연동. 호출하면 해당 레이어 스트로크만 렌더하고 즉시 리페인트. */
	UFUNCTION(BlueprintCallable, Category = "Heist|Briefing")
	void SetActiveLayer(FName InLayerId);

	UFUNCTION(BlueprintPure, Category = "Heist|Briefing")
	FName GetActiveLayerId() const { return ActiveLayerId; }

	UPROPERTY(BlueprintAssignable, Category = "Heist|Briefing")
	FOnBriefingMapInitialized OnBriefingMapInitialized;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	FVector2D AbsoluteToNormalized(const FGeometry& InGeometry, const FVector2D& InAbsolutePosition) const;
	void HandlePreviewChunkReceived(const FHeistBriefingStrokePreviewChunk& InChunk);
	void HandleStrokeCommitted(const FHeistBriefingStroke& InStroke);
	void RebuildCommittedStrokeCache();

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> PlanImage;

	UPROPERTY()
	TObjectPtr<UHeistBriefingPlayerComponent> BriefingPlayerComponent;

	UPROPERTY()
	TObjectPtr<UHeistBriefingDrawingSyncComponent> DrawingSyncComponent;

	UPROPERTY(EditAnywhere, Category = "Heist|Briefing")
	TArray<FHeistBriefingPlanLayerDefinition> PlanLayers;

	UPROPERTY()
	TArray<FHeistBriefingStroke> CachedCommittedStrokes;

	UPROPERTY()
	TArray<FHeistBriefingStrokePreviewChunk> CachedPreviewChunks;

	FHeistBriefingStroke ActiveLocalStroke;
	FHeistBriefingStrokeStyle CurrentStrokeStyle;
	EHeistBriefingViewMode ViewMode = EHeistBriefingViewMode::Thief;
	bool bIsDrawing = false;

	/** 현재 표시 중인 레이어 ID. NativePaint 필터링 + 신규 스트로크 LayerId 세팅에 사용. */
	FName ActiveLayerId = NAME_None;

};
