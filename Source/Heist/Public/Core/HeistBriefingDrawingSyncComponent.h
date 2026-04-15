
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/HeistMatchTypes.h"
#include "HeistBriefingDrawingSyncComponent.generated.h"


DECLARE_MULTICAST_DELEGATE_OneParam(FHeistBriefingPreviewChunkReceived, const FHeistBriefingStrokePreviewChunk&);
DECLARE_MULTICAST_DELEGATE_OneParam(FHeistBriefingStrokeCommitted, const FHeistBriefingStroke&);
DECLARE_MULTICAST_DELEGATE(FHeistBriefingHistoryRebuilt);

/**
 * 브리핑 보드의 stroke 이력 저장소.
 *
 * committed stroke는 replicated history로 유지하고,
 * preview chunk는 로컬 delegate로만 전달한다.
 *
 * 지우개는 정밀한 픽셀 단위 편집기가 아니라 러프한 stroke 삭제기로 동작한다.
 * 같은 작성자 / 같은 레이어의 stroke 중 지우개 경로에 넓게 겹친 stroke는
 * 전체 stroke 단위로 제거될 수 있다.
 *
 * 위젯은 이 컴포넌트를 구독해
 * - 실시간 preview 표시
 * - committed stroke 반영
 * - late join / UI 재오픈 시 history 복원
 * 을 수행한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistBriefingDrawingSyncComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(ReplicatedUsing=OnRep_StrokeHistory)
	TArray<FHeistBriefingStroke> StrokeHistory;

	UFUNCTION()
	void OnRep_StrokeHistory();

	int32 LastDeliveredCount = 0;
	
public:
	UHeistBriefingDrawingSyncComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void AppendStrokeAuthoritative(const FHeistBriefingStroke& InStroke);
	void BroadcastPreviewChunkLocal(const FHeistBriefingStrokePreviewChunk& InChunk);
	void GetStrokeHistory(TArray<FHeistBriefingStroke>& OutHistory) const;
	
	FHeistBriefingPreviewChunkReceived OnPreviewChunkReceived;
	FHeistBriefingStrokeCommitted OnStrokeCommitted; // 랜더/캐시 갱신용으로만 사용할 것
	FHeistBriefingHistoryRebuilt OnHistoryRebuilt;
};
