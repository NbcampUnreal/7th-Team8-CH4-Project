
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistPhaseManagerComponent.generated.h"

/**
 * 매치의 상위 phase 흐름을 관제하는 컴포넌트.
 *
 * PhaseManager는 "언제 무엇을 실행할지"만 담당한다.
 * BriefingPhase의 세부 운영은 UHeistBriefingPhaseComponent,
 * 브리핑 결과의 실제 적용은 UHeistExecutionPhaseComponent가 맡는다.
 *
 * 현재 책임:
 * - 매치 흐름 시작
 * - Briefing 진입
 * - Briefing 잠금 시점 진입
 * - Execution 진입
 * - 남은 시간 갱신 및 UI 알림 브로드캐스트
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistPhaseManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void StartMatchFlow();

private:
	void EnterBriefingPhase();
	void LockBriefingSelections();
	void EnterExecutionPhase();
	
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Phase")
	float BriefingDuration = 80.f; // 기본 - 1분 20초
	
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Phase")
	float LockLeadTime = 10.f; // 10초 전 잠금
	
	FTimerHandle BriefingLockTimerHandle;
	FTimerHandle BriefingTimerHandle;
};
