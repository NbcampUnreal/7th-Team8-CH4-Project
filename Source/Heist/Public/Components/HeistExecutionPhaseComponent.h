
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistExecutionPhaseComponent.generated.h"

/**
 * Briefing 이후 시작되는 실게임 본편을 담당하는 컴포넌트.
 *
 * 초기 진입 시에는 브리핑에서 확정된 결과를 월드에 반영하고,
 * 장기적으로는 Result phase에 들어가기 전까지의 경기 진행을
 * 포괄하는 위치로 확장될 수 있다.
 *
 * 현재 책임:
 * - 도둑 스폰 위치 적용
 * - 경찰 목표 오브젝트 배치
 * - 실게임 시작 활성화
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistExecutionPhaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void EnterExecutionPhase();

private:
	void ApplyThiefInsertionSpawns();
	void SpawnPoliceObjective();
	void RestoreVoiceToPawnRoot();
	void ActivateExecutionGameplay();

	/**
	 * 경찰 목표 오브젝트 액터 클래스.
	 * BP_PoliceObjective 구현 완료 후 GameMode BP에서 할당할 것.
	 * 생성할 물건을 할당할 것
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Execution")
	TSubclassOf<AActor> PoliceObjectiveClass;
};
