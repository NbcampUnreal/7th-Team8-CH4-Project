
#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "CoreMinimal.h"
#include "HeistBriefingPlayerComponent.h"
#include "Components/ActorComponent.h"
#include "Core/HeistMatchTypes.h"
#include "HeistBriefingPhaseComponent.generated.h"

class AHeistBriefingDrawingBoard;
class UGameplayEffect;
class UHeistBriefingPointDataAsset;

/**
 * Phase 0 - Briefing 전용 운영 컴포넌트.
 *
 * 이 컴포넌트는 브리핑 동안 필요한 시스템만 준비하고 정리한다.
 * 상위 phase 전환 타이밍은 UHeistPhaseManagerComponent가 담당하며,
 * 브리핑 결과의 실제 반영은 UHeistExecutionPhaseComponent가 맡는다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistBriefingPhaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void EnterBriefingPhase();
	void LockSelections();
	void ExitBriefingPhase();

	void SetThiefSpawnSelection(APlayerState* PlayerState, FName InKey);
	void SetPoliceObjectiveSelection(APlayerState* PlayerState, FName InKey);
	FName GetThiefSpawnSelection(const APlayerState* PlayerState) const;
	FName GetPoliceObjectiveSelection(const APlayerState* PlayerState) const;
	FText GetPoliceObjectiveDisplayName(FName InKey) const;

	/** 서버 RPC에서 PlayerComponent가 직접 호출. 유효성 검사 + 저장. */
	void TrySetThiefSpawnSelection(APlayerState* PlayerState, FName InKey);
	void TrySetPoliceObjectiveSelection(APlayerState* PlayerState, FName InKey);

	/** 특정 컴포넌트 하나에 현재 카운트 push. ServerRequestThiefCounts 응답용. */
	void PushThiefCountsTo(UHeistBriefingPlayerComponent* Target);

	/** 도둑 스폰 포인트 맵. ExecutionPhaseComponent에서 사용. */
	const TMap<FName, FVector>& GetThiefSpawnPointMap() const { return ThiefSpawnPointMap; }

	/** 경찰 목표 후보 위치 맵. ExecutionPhaseComponent에서 사용. */
	const TMap<FName, FVector>& GetPoliceObjectivePointMap() const { return PoliceObjectivePointMap; }

private:
	void AssignRandomRoles();
	void RequestPlayersSpawnAtBriefingStart();
	void SpawnBriefingActors();
	void BindPlayersToBriefingActors();

	void FinalizeDefaultSelections();
	void ApplyBriefingStateToPlayers();
	void RemoveBriefingStateFromPlayers();

	void GatherThiefSpawnPoints();
	void GatherPoliceObjectivePoints();

	UPROPERTY()
	TObjectPtr<AHeistBriefingDrawingBoard> DrawingBoard;

	/** BriefingPointData.WorldTag로 수집. Key는 동일 배열 항목의 Key. */
	TMap<FName, FVector> ThiefSpawnPointMap;

	/** BriefingPointData.WorldTag로 수집. Key는 동일 배열 항목의 Key. */
	TMap<FName, FVector> PoliceObjectivePointMap;

	/** 브리핑 중 각 도둑이 고른 진입 포인트 키. 서버 전용 저장소. */
	TMap<TObjectPtr<APlayerState>, FName> ThiefSpawnSelections;

	/** 브리핑 중 경찰이 고른 목표 배치 위치. 서버 전용 저장소. */
	TMap<TObjectPtr<APlayerState>, FName> PoliceObjectiveSelections;

	/**
	 * 브리핑 포인트 정의 테이블. GameMode BP에서 할당한다.
	 * 서버는 WorldTag로 레벨 액터를 수집하고, Key 유효성 검사에도 사용한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Briefing")
	TObjectPtr<UHeistBriefingPointDataAsset> BriefingPointData;

	/** 브리핑 동안 플레이어 입력/행동을 잠그는 GE. GameMode BP에서 할당한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Briefing")
	TSubclassOf<UGameplayEffect> BriefingStateEffectClass;

	/** 브리핑 잠금 GE 핸들. Execution 진입 전 제거한다. PlayerState 생명주기와 분리하기 위해 weak로 보관한다. */
	TMap<TWeakObjectPtr<APlayerState>, FActiveGameplayEffectHandle> BriefingStateEffectHandles;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Briefing")
	TSubclassOf<AHeistBriefingDrawingBoard> DrawingBoardClass;
};
