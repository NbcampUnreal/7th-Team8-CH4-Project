#include "Components/HeistGameOverPhaseComponent.h"
#include "Core/HeistMatchGameMode.h"
#include "Core/HeistMatchGameState.h"

UHeistGameOverPhaseComponent::UHeistGameOverPhaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UHeistGameOverPhaseComponent::StartEngineChanneling()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			EngineTimerHandle,
			this,
			&ThisClass::EngineChannelingEnd,
			EngineChannelingDuration,
			false);
	}
	UE_LOG(LogTemp, Warning, TEXT("엔진 채널링 시작"));
}

void UHeistGameOverPhaseComponent::EngineChannelingEnd()
{
	AHeistMatchGameMode* HeistGM = GetOwner<AHeistMatchGameMode>();
	if (!IsValid(HeistGM)) return;

	AHeistMatchGameState* HeistGS = HeistGM->GetGameState<AHeistMatchGameState>();
	if (!HeistGS) return;

	HeistGS->SetEngineChannelingEnd(true);
	UE_LOG(LogTemp, Warning, TEXT("엔진 채널링 종료"));
}
