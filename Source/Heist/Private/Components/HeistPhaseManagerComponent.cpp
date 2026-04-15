
#include "Components/HeistPhaseManagerComponent.h"

#include "Components/HeistBriefingPhaseComponent.h"
#include "Components/HeistExecutionPhaseComponent.h"
#include "Core/HeistMatchGameMode.h"
#include "Core/HeistMatchGameState.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UHeistPhaseManagerComponent::StartMatchFlow()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	EnterBriefingPhase();
}

void UHeistPhaseManagerComponent::EnterBriefingPhase()
{
	AHeistMatchGameMode* HeistGM = GetOwner<AHeistMatchGameMode>();
	if (!IsValid(HeistGM)) return;

	AHeistMatchGameState* HeistGS = HeistGM->GetGameState<AHeistMatchGameState>();
	if (IsValid(HeistGS))
	{
		HeistGS->SetCurrentPhase(EHeistMatchPhase::Briefing);
		HeistGS->SetBriefingSelectionLocked(false);
		HeistGS->SetPhaseRemainingTime(BriefingDuration);
		HeistGS->SetPhaseEndServerTime(GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + BriefingDuration);
	}

	if (UHeistBriefingPhaseComponent* BriefingPhase = HeistGM->FindComponentByClass<UHeistBriefingPhaseComponent>())
	{
		BriefingPhase->EnterBriefingPhase();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BriefingLockTimerHandle);
		World->GetTimerManager().ClearTimer(BriefingTimerHandle);

		World->GetTimerManager().SetTimer(
			BriefingLockTimerHandle,
			this,
			&ThisClass::LockBriefingSelections,
			FMath::Max(BriefingDuration - LockLeadTime, 0.f),
			false);

		World->GetTimerManager().SetTimer(
			BriefingTimerHandle,
			this,
			&ThisClass::EnterExecutionPhase,
			BriefingDuration,
			false);
	}
}

void UHeistPhaseManagerComponent::LockBriefingSelections()
{
	AHeistMatchGameMode* HeistGM = GetOwner<AHeistMatchGameMode>();
	if (!IsValid(HeistGM)) return;

	if (AHeistMatchGameState* HeistGS = HeistGM->GetGameState<AHeistMatchGameState>())
	{
		HeistGS->SetBriefingSelectionLocked(true);
	}

	if (UHeistBriefingPhaseComponent* BriefingPhase = HeistGM->FindComponentByClass<UHeistBriefingPhaseComponent>())
	{
		BriefingPhase->LockSelections();
	}
}

void UHeistPhaseManagerComponent::EnterExecutionPhase()
{
	AHeistMatchGameMode* HeistGM = GetOwner<AHeistMatchGameMode>();
	if (!IsValid(HeistGM)) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BriefingLockTimerHandle);
		World->GetTimerManager().ClearTimer(BriefingTimerHandle);
	}

	if (AHeistMatchGameState* HeistGS = HeistGM->GetGameState<AHeistMatchGameState>())
	{
		HeistGS->SetCurrentPhase(EHeistMatchPhase::Execution);
		HeistGS->SetBriefingSelectionLocked(true);
		HeistGS->SetPhaseRemainingTime(0.f);
		HeistGS->SetPhaseEndServerTime(0.f);
	}

	if (UHeistExecutionPhaseComponent* ExecutionPhase = HeistGM->FindComponentByClass<UHeistExecutionPhaseComponent>())
	{
		ExecutionPhase->EnterExecutionPhase();
	}
}
