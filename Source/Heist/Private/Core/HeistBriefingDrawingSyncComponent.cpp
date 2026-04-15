
#include "Core/HeistBriefingDrawingSyncComponent.h"

#include "Net/UnrealNetwork.h"

namespace
{
	// 지우개 판정 반경 튜닝용 상수.
	// - 더 잘 지워지게(넓게) 하려면:
	//   * EraserThicknessToNormalizedScale 값을 키운다.
	//   * 필요하면 MaxEraserHitRadiusNormalized도 키운다.
	// - 더 정밀하게(좁게) 하려면:
	//   * EraserThicknessToNormalizedScale 값을 줄인다.
	//   * 필요하면 Min/MaxEraserHitRadiusNormalized를 함께 낮춘다.
	constexpr float MinEraserHitRadiusNormalized = 0.025f;
	constexpr float MaxEraserHitRadiusNormalized = 0.05f;
	constexpr float EraserThicknessToNormalizedScale = 0.025f;

	// 브리핑 스케치용 러프 지우개 정책:
	// - 같은 작성자 / 같은 레이어의 stroke만 지운다.
	// - 정밀 픽셀 편집 대신 "가까이 지나간 stroke 전체 제거"를 우선한다.
	// - 다만 normalized 좌표계 기준 반경은 별도로 clamp해, 허공 클릭 광역 삭제는 막는다.
	bool DoesEraserStrokeHitStroke(const FHeistBriefingStroke& EraserStroke, const FHeistBriefingStroke& TargetStroke)
	{
		if (TargetStroke.bRemoved
			|| TargetStroke.LayerId != EraserStroke.LayerId
			|| TargetStroke.AuthorPlayerName != EraserStroke.AuthorPlayerName)
		{
			return false;
		}

		const float EraserRadius = FMath::Clamp(
			EraserStroke.Style.Thickness * EraserThicknessToNormalizedScale,
			MinEraserHitRadiusNormalized,
			MaxEraserHitRadiusNormalized);
		const float EraserRadiusSq = FMath::Square(EraserRadius);

		for (const FHeistBriefingStrokePoint& EraserPoint : EraserStroke.Points)
		{
			for (const FHeistBriefingStrokePoint& TargetPoint : TargetStroke.Points)
			{
				if (FVector2D::DistSquared(EraserPoint.NormalizedPosition, TargetPoint.NormalizedPosition) <= EraserRadiusSq)
				{
					return true;
				}
			}
		}

		return false;
	}
}

UHeistBriefingDrawingSyncComponent::UHeistBriefingDrawingSyncComponent()
{
	SetIsReplicatedByDefault(true);
}

void UHeistBriefingDrawingSyncComponent::GetLifetimeReplicatedProps(
	TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHeistBriefingDrawingSyncComponent, StrokeHistory);
}

void UHeistBriefingDrawingSyncComponent::OnRep_StrokeHistory()
{
	if (LastDeliveredCount > StrokeHistory.Num())
	{
		LastDeliveredCount = 0;
		OnHistoryRebuilt.Broadcast();
	}

	for (int32 Index = LastDeliveredCount; Index < StrokeHistory.Num(); ++Index)
	{
		OnStrokeCommitted.Broadcast(StrokeHistory[Index]);
	}

	LastDeliveredCount = StrokeHistory.Num();
}

void UHeistBriefingDrawingSyncComponent::AppendStrokeAuthoritative(const FHeistBriefingStroke& InStroke)
{
	if (InStroke.Style.Tool == EHeistBriefingDrawingTool::Eraser)
	{
		// 지우개 stroke 자체는 history에 남기지 않고, 겹치는 기존 stroke를 즉시 제거한다.
		bool bAnyStrokeRemoved = false;

		for (FHeistBriefingStroke& ExistingStroke : StrokeHistory)
		{
			if (DoesEraserStrokeHitStroke(InStroke, ExistingStroke))
			{
				ExistingStroke.bRemoved = true;
				bAnyStrokeRemoved = true;
			}
		}

		if (bAnyStrokeRemoved)
		{
			StrokeHistory.RemoveAll(
				[](const FHeistBriefingStroke& Stroke)
				{
					return Stroke.bRemoved;
				});

			LastDeliveredCount = 0;
			OnHistoryRebuilt.Broadcast();
		}

		return;
	}

	StrokeHistory.Add(InStroke);
	OnStrokeCommitted.Broadcast(InStroke);
	LastDeliveredCount = StrokeHistory.Num();
}

void UHeistBriefingDrawingSyncComponent::BroadcastPreviewChunkLocal(const FHeistBriefingStrokePreviewChunk& InChunk)
{
	OnPreviewChunkReceived.Broadcast(InChunk);
}

void UHeistBriefingDrawingSyncComponent::GetStrokeHistory(TArray<FHeistBriefingStroke>& OutHistory) const
{
	OutHistory = StrokeHistory;
}
