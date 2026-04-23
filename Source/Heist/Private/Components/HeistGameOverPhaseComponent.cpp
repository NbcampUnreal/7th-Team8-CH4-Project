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
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("엔진이 작동을 시작했습니다!"));
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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("출발 준비 완료! 문을 닫고 출발하세요!"));
	}
	UE_LOG(LogTemp, Warning, TEXT("엔진 채널링 종료"));
}
