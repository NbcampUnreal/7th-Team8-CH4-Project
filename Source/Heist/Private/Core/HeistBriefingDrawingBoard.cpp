
#include "Core/HeistBriefingDrawingBoard.h"

#include "Core/HeistBriefingDrawingSyncComponent.h"

AHeistBriefingDrawingBoard::AHeistBriefingDrawingBoard()
{
	// 모든 클라이언트에 복제돼야 DrawingBoard 참조가 클라이언트에서 유효해진다.
	// MulticastReceivePreviewChunk, StrokeHistory 복제도 이 플래그에 의존한다.

	bReplicates = true;
	bAlwaysRelevant = true;
	//SetReplicates(true);
	//bAlwaysRelevant = true;

	DrawingSyncComponent = CreateDefaultSubobject<UHeistBriefingDrawingSyncComponent>(TEXT("DrawingSyncComponent"));
}

bool AHeistBriefingDrawingBoard::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget,
	const FVector& SrcLocation) const
{
	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

void AHeistBriefingDrawingBoard::BroadcastPreviewChunkAuthoritative(const FHeistBriefingStrokePreviewChunk& InChunk)
{
	MulticastReceivePreviewChunk(InChunk);
}

void AHeistBriefingDrawingBoard::CommitStrokeAuthoritative(const FHeistBriefingStroke& InStroke)
{
	if (!IsValid(DrawingSyncComponent))
	{
		return;
	}

	DrawingSyncComponent->AppendStrokeAuthoritative(InStroke);
}

void AHeistBriefingDrawingBoard::MulticastReceivePreviewChunk_Implementation(
	const FHeistBriefingStrokePreviewChunk& InChunk)
{
	if (!IsValid(DrawingSyncComponent))
	{
		return;
	}

	DrawingSyncComponent->BroadcastPreviewChunkLocal(InChunk);
}
