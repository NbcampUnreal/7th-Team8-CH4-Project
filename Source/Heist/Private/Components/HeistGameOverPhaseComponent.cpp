#include "Components/HeistGameOverPhaseComponent.h"
#include "Core/HeistMatchGameMode.h"
#include "Core/HeistMatchGameState.h"
#include "TimerManager.h"

UHeistGameOverPhaseComponent::UHeistGameOverPhaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UHeistGameOverPhaseComponent::StartEngineChanneling()
{
	AHeistMatchGameMode* HeistGM = GetOwner<AHeistMatchGameMode>();
	if (!IsValid(HeistGM)) return;

	AHeistMatchGameState* HeistGS = HeistGM->GetGameState<AHeistMatchGameState>();
	if (!IsValid(HeistGS)) return;

	HeistGS->SetEngineChannelingStart(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			EngineTimerHandle,
			this,
			&ThisClass::EngineChannelingEnd,
			EngineChannelingDuration,
			false);
	}

	UE_LOG(LogTemp, Log, TEXT("[GameOverPhase] 엔진 채널링 시작"));
}

void UHeistGameOverPhaseComponent::EngineChannelingEnd()
{
	AHeistMatchGameMode* HeistGM = GetOwner<AHeistMatchGameMode>();
	if (!IsValid(HeistGM)) return;

	AHeistMatchGameState* HeistGS = HeistGM->GetGameState<AHeistMatchGameState>();
	if (!IsValid(HeistGS)) return;

	HeistGS->SetEngineChannelingEnd(true);

	UE_LOG(LogTemp, Log, TEXT("[GameOverPhase] 엔진 채널링 종료"));
}
