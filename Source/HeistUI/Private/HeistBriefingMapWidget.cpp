
#include "HeistBriefingMapWidget.h"

#include "Components/HeistBriefingPlayerComponent.h"
#include "Core/HeistBriefingDrawingSyncComponent.h"

#include "InputCoreTypes.h"
#include "GameFramework/PlayerState.h"
#include "Input/Reply.h"

namespace
{
	bool IsStrokeVisibleToViewer(
		const FString& ViewerPlayerName,
		const EHeistBriefingViewMode ViewerMode,
		const FHeistBriefingStroke& Stroke)
	{
		switch (Stroke.VisibilityScope)
		{
		case EHeistBriefingStrokeVisibilityScope::ThiefTeamShared:
			return ViewerMode == EHeistBriefingViewMode::Thief;

		case EHeistBriefingStrokeVisibilityScope::PolicePrivate:
			return ViewerMode == EHeistBriefingViewMode::Police
				&& Stroke.AuthorPlayerName == ViewerPlayerName;

		default:
			return false;
		}
	}

	bool IsPreviewVisibleToViewer(
		const FString& ViewerPlayerName,
		const EHeistBriefingViewMode ViewerMode,
		const FHeistBriefingStrokePreviewChunk& Chunk)
	{
		switch (Chunk.VisibilityScope)
		{
		case EHeistBriefingStrokeVisibilityScope::ThiefTeamShared:
			return ViewerMode == EHeistBriefingViewMode::Thief;

		case EHeistBriefingStrokeVisibilityScope::PolicePrivate:
			return ViewerMode == EHeistBriefingViewMode::Police
				&& Chunk.AuthorPlayerName == ViewerPlayerName;

		default:
			return false;
		}
	}
}

void UHeistBriefingMapWidget::InitializeForBriefing(
	UHeistBriefingPlayerComponent* InBriefingPlayerComponent,
	UHeistBriefingDrawingSyncComponent* InDrawingSyncComponent,
	EHeistBriefingViewMode InViewMode)
{
	if (IsValid(DrawingSyncComponent))
	{
		DrawingSyncComponent->OnPreviewChunkReceived.RemoveAll(this);
		DrawingSyncComponent->OnStrokeCommitted.RemoveAll(this);
		DrawingSyncComponent->OnHistoryRebuilt.RemoveAll(this);
	}

	BriefingPlayerComponent = InBriefingPlayerComponent;
	DrawingSyncComponent = InDrawingSyncComponent;
	ViewMode = InViewMode;
	CurrentStrokeStyle = FHeistBriefingStrokeStyle{};

	// 첫 번째 레이어를 기본 활성 레이어로 설정
	if (!PlanLayers.IsEmpty())
	{
		ActiveLayerId = PlanLayers[0].LayerId;
		ApplyPlanTextureForLayer(PlanLayers[0]);
	}

	if (IsValid(DrawingSyncComponent))
	{
		DrawingSyncComponent->OnPreviewChunkReceived.AddUObject(this, &ThisClass::HandlePreviewChunkReceived);
		DrawingSyncComponent->OnStrokeCommitted.AddUObject(this, &ThisClass::HandleStrokeCommitted);
		DrawingSyncComponent->OnHistoryRebuilt.AddUObject(this, &ThisClass::RebuildCommittedStrokeCache);
	}

	RebuildCommittedStrokeCache();

	OnBriefingMapInitialized.Broadcast(BriefingPlayerComponent);
}

void UHeistBriefingMapWidget::SetCurrentDrawingTool(EHeistBriefingDrawingTool InTool)
{
	CurrentStrokeStyle.Tool = InTool;
}

void UHeistBriefingMapWidget::SetActiveLayer(FName InLayerId)
{
	if (ActiveLayerId == InLayerId) return;

	ActiveLayerId = InLayerId;

	const FHeistBriefingPlanLayerDefinition* ActiveLayer = PlanLayers.FindByPredicate(
		[this](const FHeistBriefingPlanLayerDefinition& L){ return L.LayerId == ActiveLayerId; });

	if (ActiveLayer)
	{
		ApplyPlanTextureForLayer(*ActiveLayer);
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

void UHeistBriefingMapWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UHeistBriefingMapWidget::NativeDestruct()
{
	if (IsValid(DrawingSyncComponent))
	{
		DrawingSyncComponent->OnPreviewChunkReceived.RemoveAll(this);
		DrawingSyncComponent->OnStrokeCommitted.RemoveAll(this);
		DrawingSyncComponent->OnHistoryRebuilt.RemoveAll(this);
	}

	Super::NativeDestruct();
}

int32 UHeistBriefingMapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FString ViewerPlayerName = GetOwningPlayerState() ? GetOwningPlayerState()->GetPlayerName() : FString();

	// 스트로크 → 로컬 픽셀 좌표 배열로 변환 후 선 렌더
	auto DrawStroke = [&](const TArray<FHeistBriefingStrokePoint>& Points, const FHeistBriefingStrokeStyle& Style)
	{
		if (Style.Tool == EHeistBriefingDrawingTool::Eraser) return;
		if (Points.Num() < 2) return;

		TArray<FVector2D> PixelPoints;
		PixelPoints.Reserve(Points.Num());
		for (const FHeistBriefingStrokePoint& Pt : Points)
		{
			PixelPoints.Add(Pt.NormalizedPosition * Size);
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			PixelPoints,
			ESlateDrawEffect::None,
			Style.Color,
			true,
			Style.Thickness);
	};

	// Committed strokes — 활성 레이어만 렌더 (ActiveLayerId == NAME_None 이면 전체)
	for (const FHeistBriefingStroke& Stroke : CachedCommittedStrokes)
	{
		if (ActiveLayerId != NAME_None && Stroke.LayerId != ActiveLayerId) continue;
		if (!IsStrokeVisibleToViewer(ViewerPlayerName, ViewMode, Stroke)) continue;
		DrawStroke(Stroke.Points, Stroke.Style);
	}

	// Preview chunks — StrokeId별로 묶어서 렌더
	TMap<FGuid, TArray<const FHeistBriefingStrokePreviewChunk*>> ChunksByStroke;
	for (const FHeistBriefingStrokePreviewChunk& Chunk : CachedPreviewChunks)
	{
		if (!IsPreviewVisibleToViewer(ViewerPlayerName, ViewMode, Chunk)) continue;
		ChunksByStroke.FindOrAdd(Chunk.StrokeId).Add(&Chunk);
	}
	for (const auto& [StrokeId, Chunks] : ChunksByStroke)
	{
		if (Chunks.IsEmpty()) continue;
		if (ActiveLayerId != NAME_None && Chunks[0]->LayerId != ActiveLayerId) continue;

		TArray<FHeistBriefingStrokePoint> MergedPoints;
		for (const FHeistBriefingStrokePreviewChunk* Chunk : Chunks)
		{
			MergedPoints.Append(Chunk->ChunkPoints);
		}
		DrawStroke(MergedPoints, Chunks[0]->Style);
	}

	// 로컬에서 그리는 중인 스트로크
	if (bIsDrawing && ActiveLocalStroke.Points.Num() >= 2)
	{
		DrawStroke(ActiveLocalStroke.Points, ActiveLocalStroke.Style);
	}

	return LayerId;
}

FReply UHeistBriefingMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!IsValid(BriefingPlayerComponent))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FKey PressedButton = InMouseEvent.GetEffectingButton();
	if (PressedButton != EKeys::LeftMouseButton && PressedButton != EKeys::RightMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	bIsDrawing = true;
	ActiveLocalStroke = FHeistBriefingStroke{};
	ActiveLocalStroke.StrokeID = FGuid::NewGuid();
	ActiveLocalStroke.LayerId = ActiveLayerId;
	ActiveLocalStroke.Style = CurrentStrokeStyle;
	ActiveLocalStroke.Style.Tool =
		PressedButton == EKeys::LeftMouseButton
			? EHeistBriefingDrawingTool::Pen
			: EHeistBriefingDrawingTool::Eraser;
	ActiveLocalStroke.Points.Add({ AbsoluteToNormalized(InGeometry, InMouseEvent.GetScreenSpacePosition()), 0.f });

	return FReply::Handled();
}

FReply UHeistBriefingMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bIsDrawing || !IsValid(BriefingPlayerComponent))
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	const FVector2D NormalizedPosition = AbsoluteToNormalized(InGeometry, InMouseEvent.GetScreenSpacePosition());
	ActiveLocalStroke.Points.Add({ NormalizedPosition, 0.f });

	FHeistBriefingStrokePreviewChunk PreviewChunk;
	PreviewChunk.StrokeId = ActiveLocalStroke.StrokeID;
	PreviewChunk.LayerId = ActiveLayerId;
	PreviewChunk.Style = ActiveLocalStroke.Style;
	PreviewChunk.ChunkPoints.Add({ NormalizedPosition, 0.f });

	BriefingPlayerComponent->ServerPushPreviewChunk(PreviewChunk);
	return FReply::Handled();
}

FReply UHeistBriefingMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bIsDrawing || !IsValid(BriefingPlayerComponent))
	{
		bIsDrawing = false;
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	bIsDrawing = false;
	ActiveLocalStroke.Points.Add({ AbsoluteToNormalized(InGeometry, InMouseEvent.GetScreenSpacePosition()), 0.f });

	if (ActiveLocalStroke.Points.Num() > 0)
	{
		BriefingPlayerComponent->ServerCommitStroke(ActiveLocalStroke);
	}

	ActiveLocalStroke = FHeistBriefingStroke{};
	return FReply::Handled();
}

FVector2D UHeistBriefingMapWidget::AbsoluteToNormalized(const FGeometry& InGeometry, const FVector2D& InAbsolutePosition) const
{
	const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InAbsolutePosition);
	const FVector2D LocalSize = InGeometry.GetLocalSize();

	if (LocalSize.X <= 0.f || LocalSize.Y <= 0.f)
	{
		return FVector2D::ZeroVector;
	}

	return FVector2D(
		FMath::Clamp(LocalPosition.X / LocalSize.X, 0.f, 1.f),
		FMath::Clamp(LocalPosition.Y / LocalSize.Y, 0.f, 1.f));
}

void UHeistBriefingMapWidget::HandlePreviewChunkReceived(const FHeistBriefingStrokePreviewChunk& InChunk)
{
	const FString ViewerPlayerName = GetOwningPlayerState() ? GetOwningPlayerState()->GetPlayerName() : FString();
	if (!IsPreviewVisibleToViewer(ViewerPlayerName, ViewMode, InChunk))
	{
		return;
	}

	CachedPreviewChunks.Add(InChunk);
}

void UHeistBriefingMapWidget::HandleStrokeCommitted(const FHeistBriefingStroke& InStroke)
{
	const FString ViewerPlayerName = GetOwningPlayerState() ? GetOwningPlayerState()->GetPlayerName() : FString();
	if (!InStroke.bRemoved && IsStrokeVisibleToViewer(ViewerPlayerName, ViewMode, InStroke))
	{
		CachedCommittedStrokes.Add(InStroke);
	}

	CachedPreviewChunks.RemoveAll(
		[&InStroke](const FHeistBriefingStrokePreviewChunk& Chunk)
		{
			return Chunk.StrokeId == InStroke.StrokeID;
		});
}

void UHeistBriefingMapWidget::RebuildCommittedStrokeCache()
{
	CachedCommittedStrokes.Reset();
	CachedPreviewChunks.Reset();

	if (IsValid(DrawingSyncComponent))
	{
		TArray<FHeistBriefingStroke> AllStrokes;
		DrawingSyncComponent->GetStrokeHistory(AllStrokes);

		const FString ViewerPlayerName = GetOwningPlayerState() ? GetOwningPlayerState()->GetPlayerName() : FString();
		for (const FHeistBriefingStroke& Stroke : AllStrokes)
		{
			if (IsStrokeVisibleToViewer(ViewerPlayerName, ViewMode, Stroke))
			{
				CachedCommittedStrokes.Add(Stroke);
			}
		}
	}
}

UTexture2D* UHeistBriefingMapWidget::ResolvePlanTextureForViewMode(const FHeistBriefingPlanLayerDefinition& Layer) const
{
	switch (ViewMode)
	{
	case EHeistBriefingViewMode::Police:
		if (IsValid(Layer.PolicePlanTexture)) return Layer.PolicePlanTexture;
		if (IsValid(Layer.ThiefPlanTexture)) return Layer.ThiefPlanTexture; // fallback
		break;

	case EHeistBriefingViewMode::Thief:
	default:
		if (IsValid(Layer.ThiefPlanTexture)) return Layer.ThiefPlanTexture;
		if (IsValid(Layer.PolicePlanTexture)) return Layer.PolicePlanTexture; // fallback
		break;
	}

	return nullptr;
}

void UHeistBriefingMapWidget::ApplyPlanTextureForLayer(const FHeistBriefingPlanLayerDefinition& Layer)
{
	if (!IsValid(PlanImage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BriefingMap] PlanImage is null."));
		return;
	}

	if (UTexture2D* Texture = ResolvePlanTextureForViewMode(Layer))
	{
		PlanImage->SetBrushFromTexture(Texture);
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[BriefingMap] Missing plan texture. LayerId=%s ViewMode=%d"),
		*Layer.LayerId.ToString(),
		static_cast<int32>(ViewMode));

	PlanImage->SetBrush(FSlateBrush());
}
