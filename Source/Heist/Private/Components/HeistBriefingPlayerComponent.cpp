
#include "Components/HeistBriefingPlayerComponent.h"

#include "Components/HeistBriefingPhaseComponent.h"
#include "Core/HeistBriefingDrawingBoard.h"
#include "Core/HeistBriefingDrawingSyncComponent.h"
#include "Core/HeistPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

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

	// 서버는 플레이어별 컨텍스트를 먼저 발행하고, 클라에서는 복제가 도착할 때마다 다시 readiness를 평가한다.
	UE_LOG(LogTemp, Log, TEXT("[BriefingContext] Initialize: Owner=%s Board=%s ViewMode=%d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(DrawingBoard),
		static_cast<int32>(ViewMode));
	BroadcastContextReady();
}

void UHeistBriefingPlayerComponent::OnRep_DrawingBoard()
{
	UE_LOG(LogTemp, Log, TEXT("[BriefingContext] OnRep_DrawingBoard: Owner=%s Board=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(DrawingBoard));
	BroadcastContextReady();
}

void UHeistBriefingPlayerComponent::BroadcastContextReady()
{
	if (!IsValid(DrawingBoard))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[BriefingContext] Broadcast blocked: DrawingBoard invalid"));
		return;
	}

	UHeistBriefingDrawingSyncComponent* DrawingSyncComponent = DrawingBoard->GetDrawingSyncComponent();
	if (!IsValid(DrawingSyncComponent))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[BriefingContext] Broadcast blocked: DrawingSync invalid Board=%s"),
			*GetNameSafe(DrawingBoard));
		return;
	}

	// 서버는 팀 -> 폰 스폰 -> 브리핑 컨텍스트 순서로 상태를 만든다.
	// 클라는 이 결과를 각기 다른 시점에 복제로 받으므로, 팀/컨트롤러/폰이 모두 준비됐을 때만
	// 브리핑 UI 시작 메시지를 발사해야 한다.
	AHeistPlayerState* OwnerPS = Cast<AHeistPlayerState>(GetOwner());
	if (!IsValid(OwnerPS))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[BriefingContext] Broadcast blocked: Owner PlayerState invalid"));
		return;
	}

	if (OwnerPS->GetAssignedTeam() == EHeistTeam::None)
	{
		UE_LOG(LogTemp, Log, TEXT("[BriefingContext] Broadcast deferred: Team not assigned yet Owner=%s"),
			*OwnerPS->GetName());
		return;
	}

	APlayerController* PC = OwnerPS->GetPlayerController();
	if (!IsValid(PC) || !PC->IsLocalController())
	{
		UE_LOG(LogTemp, Log, TEXT("[BriefingContext] Broadcast deferred: Local PC not ready Owner=%s PC=%s"),
			*OwnerPS->GetName(),
			*GetNameSafe(PC));
		return;
	}

	APawn* Pawn = PC->GetPawn();
	if (!IsValid(Pawn))
	{
		UE_LOG(LogTemp, Log, TEXT("[BriefingContext] Broadcast deferred: Pawn not possessed yet PC=%s"),
			*PC->GetName());
		return;
	}

	FHeistBriefingContextReadyMessage Msg;
	Msg.BriefingPlayerComponent = this;
	Msg.DrawingSyncComponent = DrawingSyncComponent;
	Msg.ViewMode = ViewMode;
	Msg.WidgetClass = DrawingBoard->GetBriefingWidgetClass();

	UE_LOG(LogTemp, Log, TEXT("[BriefingContext] Ready: PS=%s Team=%d PC=%s Pawn=%s Board=%s"),
		*OwnerPS->GetName(),
		static_cast<int32>(OwnerPS->GetAssignedTeam()),
		*PC->GetName(),
		*Pawn->GetName(),
		*DrawingBoard->GetName());
	UHeistMessageSubsystem::Get(this).BroadcastMessage(
		HeistMessageTags::Message_Briefing_ContextReady, Msg);
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

void UHeistBriefingPlayerComponent::ServerRequestThiefCounts_Implementation()
{
	if (BriefingPhase.IsValid())
		BriefingPhase->PushThiefCountsTo(this);
}

void UHeistBriefingPlayerComponent::ClientReceiveThiefCounts_Implementation(
	const TArray<FHeistBriefingSelectionCount>& Counts)
{
	OnThiefSelectionCountsReceived.Broadcast(Counts);
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
