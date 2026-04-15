
#include "Components/HeistBriefingPlayerComponent.h"

#include "Components/HeistBriefingPhaseComponent.h"
#include "Core/HeistBriefingDrawingBoard.h"
#include "Core/HeistPlayerState.h"
#include "Net/UnrealNetwork.h"

UHeistBriefingPlayerComponent::UHeistBriefingPlayerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UHeistBriefingPlayerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UHeistBriefingPlayerComponent, DrawingBoard, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UHeistBriefingPlayerComponent, ViewMode, COND_OwnerOnly);
}

void UHeistBriefingPlayerComponent::InitializeBriefingContext(
	AHeistBriefingDrawingBoard* InDrawingBoard,
	EHeistBriefingViewMode InViewMode)
{
	DrawingBoard = InDrawingBoard;
	ViewMode = InViewMode;

	// 서버(리슨호스트 포함) 로컬에서도 준비 신호 발동
	OnBriefingContextReady.Broadcast();
}


void UHeistBriefingPlayerComponent::OnRep_DrawingBoard()
{
	if (IsValid(DrawingBoard))
	{
		OnBriefingContextReady.Broadcast();
	}
}

void UHeistBriefingPlayerComponent::ServerPushPreviewChunk_Implementation(
	const FHeistBriefingStrokePreviewChunk& InChunk)
{
	if (!IsValid(DrawingBoard))
	{
		return;
	}

	FHeistBriefingStrokePreviewChunk ChunkToSend = InChunk;

	if (const AHeistPlayerState* HeistPlayerState = Cast<AHeistPlayerState>(GetOwner()))
	{
		ChunkToSend.AuthorPlayerName = HeistPlayerState->GetPlayerName();
		ChunkToSend.VisibilityScope = HeistPlayerState->IsPolice()
			? EHeistBriefingStrokeVisibilityScope::PolicePrivate
			: EHeistBriefingStrokeVisibilityScope::ThiefTeamShared;
	}

	DrawingBoard->BroadcastPreviewChunkAuthoritative(ChunkToSend);
}

void UHeistBriefingPlayerComponent::ServerCommitStroke_Implementation(const FHeistBriefingStroke& InStroke)
{
	if (!IsValid(DrawingBoard))
	{
		return;
	}

	FHeistBriefingStroke StrokeToCommit = InStroke;

	if (const AHeistPlayerState* HeistPlayerState = Cast<AHeistPlayerState>(GetOwner()))
	{
		StrokeToCommit.AuthorPlayerName = HeistPlayerState->GetPlayerName();
		StrokeToCommit.VisibilityScope = HeistPlayerState->IsPolice()
			? EHeistBriefingStrokeVisibilityScope::PolicePrivate
			: EHeistBriefingStrokeVisibilityScope::ThiefTeamShared;
	}

	DrawingBoard->CommitStrokeAuthoritative(StrokeToCommit);
}

void UHeistBriefingPlayerComponent::SetBriefingPhase(UHeistBriefingPhaseComponent* InPhase)
{
	BriefingPhase = InPhase;
}

void UHeistBriefingPlayerComponent::ServerSetThiefSpawnPoint_Implementation(FName InKey)
{
	AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(GetOwner());
	if (!IsValid(HeistPS) || !HeistPS->IsThief()) return;
	if (!BriefingPhase.IsValid()) return;

	BriefingPhase->TrySetThiefSpawnSelection(HeistPS, InKey);
}

void UHeistBriefingPlayerComponent::ServerSetPoliceObjectiveKey_Implementation(FName InKey)
{
	AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(GetOwner());
	if (!IsValid(HeistPS) || !HeistPS->IsPolice()) return;
	if (!BriefingPhase.IsValid()) return;

	BriefingPhase->TrySetPoliceObjectiveSelection(HeistPS, InKey);
}
