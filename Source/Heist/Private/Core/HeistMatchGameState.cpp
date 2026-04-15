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
	FHeistPhaseChangedMessage Message;
	Message.CurrentPhase = CurrentPhase;
	Message.bBriefingSelectionLocked = bBriefingSelectionLocked;

	UHeistMessageSubsystem::Get(this).BroadcastMessage(
		HeistMessageTags::Message_Phase_Changed,
		Message); // UI
}

void AHeistMatchGameState::OnRep_BriefingSelectionLocked()
{
	FHeistPhaseChangedMessage Message;
	Message.CurrentPhase = CurrentPhase;
	Message.bBriefingSelectionLocked = bBriefingSelectionLocked;

	UHeistMessageSubsystem::Get(this).BroadcastMessage(
		HeistMessageTags::Message_Phase_Changed,
		Message); // UI
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
	const float RemainingTime = FMath::Max(PhaseEndServerTime - GetServerWorldTimeSeconds(), 0.f);

	FHeistPhaseTimeUpdatedMessage Message;
	Message.RemainingTime = RemainingTime;

	UHeistMessageSubsystem::Get(this).BroadcastMessage(
		HeistMessageTags::Message_Phase_TimeUpdated,
		Message); // UI

	if (RemainingTime <= 0.f)
	{
		StopPhaseUiTimer();
	}
}
