#include "Core/HeistMatchGameState.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "TimerManager.h"

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
}

void AHeistMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeistMatchGameState, CurrentPhase);
	DOREPLIFETIME(AHeistMatchGameState, PhaseRemainingTime);
	DOREPLIFETIME(AHeistMatchGameState, bBriefingSelectionLocked);
	DOREPLIFETIME(AHeistMatchGameState, PhaseEndServerTime);
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
