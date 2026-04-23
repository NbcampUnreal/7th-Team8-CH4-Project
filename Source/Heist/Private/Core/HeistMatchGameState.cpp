#include "Core/HeistMatchGameState.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "TimerManager.h"

void AHeistMatchGameState::InitZoneScores(int32 ZoneVolumeCount, int32 TargetScore)
{
	if (!HasAuthority()) return;

	ZoneScores.Empty();
	ZoneScores.Reserve(ZoneVolumeCount);

	for (int32 i = 0; i < ZoneVolumeCount; ++i)
	{
		FZoneScoreData NewData;
		NewData.CurrentScore = 0.f;
		NewData.TargetScore = TargetScore;
		NewData.ZoneIndex = i;

		ZoneScores.Add(NewData);
	}
}

void AHeistMatchGameState::SetCurrentPhase(EHeistMatchPhase InPhase)
{
	if (CurrentPhase == InPhase)
	{
		return;
	}

	CurrentPhase = InPhase;
	OnRep_MatchPhase();
}

void AHeistMatchGameState::SetPhaseRemainingTime(float InRemainingTime)
{
	PhaseRemainingTime = InRemainingTime;
}

void AHeistMatchGameState::SetBriefingSelectionLocked(bool bLocked)
{
	bBriefingSelectionLocked = bLocked;
	OnRep_BriefingSelectionLocked(); // 서버 로컬에도 발동
}

void AHeistMatchGameState::SetPhaseEndServerTime(float InPhaseEndServerTime)
{
	PhaseEndServerTime = InPhaseEndServerTime;
	OnRep_PhaseEndServerTime();
}

void AHeistMatchGameState::SetZoneScore(int32 ZoneIndex, int32 NewScore)
{
	if (ZoneScores.IsValidIndex(ZoneIndex))
	{
		ZoneScores[ZoneIndex].CurrentScore = NewScore;
		OnRep_ZoneScores();

		UE_LOG(LogTemp, Log, TEXT("Zone %d Score Updated: %d"), ZoneIndex, NewScore);
	}
}

void AHeistMatchGameState::SetPoliceObjectiveDisplayName(const FText& InDisplayName)
{
	PoliceObjectiveDisplayName = InDisplayName;
	OnRep_PoliceObjectiveDisplayName();
}

void AHeistMatchGameState::SetEngineChannelingStart(bool bStart)
{
	bEngineChannelingStarted = bStart;
	OnRep_EngineChannelingStarted();
}

void AHeistMatchGameState::SetEngineChannelingEnd(bool bEnd)
{
	bEngineChannelingEnded = bEnd;
	OnRep_EngineChannelingEnded();
}

void AHeistMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeistMatchGameState, CurrentPhase);
	DOREPLIFETIME(AHeistMatchGameState, PhaseRemainingTime);
	DOREPLIFETIME(AHeistMatchGameState, bBriefingSelectionLocked);
	DOREPLIFETIME(AHeistMatchGameState, PhaseEndServerTime);
	DOREPLIFETIME(AHeistMatchGameState, ZoneScores);
	DOREPLIFETIME(AHeistMatchGameState, PoliceObjectiveDisplayName);
	DOREPLIFETIME(AHeistMatchGameState, bEngineChannelingStarted);
	DOREPLIFETIME(AHeistMatchGameState, bEngineChannelingEnded);
}

void AHeistMatchGameState::OnRep_MatchPhase()
{
	UHeistMessageSubsystem* Subsystem = UHeistMessageSubsystem::TryGet(this);
	if (!IsValid(Subsystem)) return;

	FHeistPhaseChangedMessage Message;
	Message.CurrentPhase = CurrentPhase;
	Message.bBriefingSelectionLocked = bBriefingSelectionLocked;

	Subsystem->BroadcastMessage(HeistMessageTags::Message_Phase_Changed, Message);
}

void AHeistMatchGameState::OnRep_BriefingSelectionLocked()
{
	UHeistMessageSubsystem* Subsystem = UHeistMessageSubsystem::TryGet(this);
	if (!IsValid(Subsystem)) return;

	FHeistPhaseChangedMessage Message;
	Message.CurrentPhase = CurrentPhase;
	Message.bBriefingSelectionLocked = bBriefingSelectionLocked;

	Subsystem->BroadcastMessage(HeistMessageTags::Message_Phase_Changed, Message);
}

void AHeistMatchGameState::OnRep_PhaseEndServerTime()
{
	BroadcastPhaseTimeUpdated();

	if (PhaseEndServerTime > 0.f)
	{
		StartPhaseUiTimer();
	}
	else
	{
		StopPhaseUiTimer();
	}
}

void AHeistMatchGameState::OnRep_ZoneScores()
{
	UHeistMessageSubsystem* Subsystem = UHeistMessageSubsystem::TryGet(this);
	if (!IsValid(Subsystem)) return;

	FHeistZoneScoresUpdatedMessage Message;
	Message.ZoneScores = ZoneScores;

	Subsystem->BroadcastMessage(HeistMessageTags::Message_PlayHUD_ZoneScoresUpdated, Message);
}

void AHeistMatchGameState::OnRep_EngineChannelingStarted()
{
	UHeistMessageSubsystem* Subsystem = UHeistMessageSubsystem::TryGet(this);
	if (!IsValid(Subsystem)) return;

	FHeistGameNotificationMessage Message;
	Message.Text = NSLOCTEXT("HeistMatchGameState", "EngineStarted", "엔진이 작동을 시작했습니다!");
	Subsystem->BroadcastMessage(HeistMessageTags::Message_UI_GameNotification, Message);
}

void AHeistMatchGameState::OnRep_EngineChannelingEnded()
{
	UHeistMessageSubsystem* Subsystem = UHeistMessageSubsystem::TryGet(this);
	if (!IsValid(Subsystem)) return;

	FHeistGameNotificationMessage Message;
	Message.Text = NSLOCTEXT("HeistMatchGameState", "EngineReady", "출발 준비 완료! 문을 닫고 출발하세요!");
	Subsystem->BroadcastMessage(HeistMessageTags::Message_UI_GameNotification, Message);
}

void AHeistMatchGameState::OnRep_PoliceObjectiveDisplayName()
{
	UHeistMessageSubsystem* Subsystem = UHeistMessageSubsystem::TryGet(this);
	if (!IsValid(Subsystem)) return;

	FHeistPoliceObjectiveUpdatedMessage Message;
	Message.DisplayName = PoliceObjectiveDisplayName;

	Subsystem->BroadcastMessage(HeistMessageTags::Message_PlayHUD_PoliceObjectiveUpdated, Message);
}

void AHeistMatchGameState::StartPhaseUiTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(PhaseUiUpdateTimerHandle);
	World->GetTimerManager().SetTimer(
		PhaseUiUpdateTimerHandle,
		this,
		&ThisClass::BroadcastPhaseTimeUpdated,
		1.0f,
		true);
}

void AHeistMatchGameState::StopPhaseUiTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseUiUpdateTimerHandle);
	}
}

void AHeistMatchGameState::BroadcastPhaseTimeUpdated()
{
	UHeistMessageSubsystem* Subsystem = UHeistMessageSubsystem::TryGet(this);
	if (!IsValid(Subsystem)) return;

	const float RemainingTime = FMath::Max(PhaseEndServerTime - GetServerWorldTimeSeconds(), 0.f);

	FHeistPhaseTimeUpdatedMessage Message;
	Message.RemainingTime = RemainingTime;

	Subsystem->BroadcastMessage(HeistMessageTags::Message_Phase_TimeUpdated, Message);

	if (RemainingTime <= 0.f)
	{
		StopPhaseUiTimer();
	}
}
