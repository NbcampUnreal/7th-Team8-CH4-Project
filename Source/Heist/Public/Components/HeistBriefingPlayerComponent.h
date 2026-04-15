
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/HeistMatchTypes.h"
#include "HeistBriefingPlayerComponent.generated.h"

class AHeistBriefingDrawingBoard;
class UHeistBriefingPhaseComponent;

DECLARE_MULTICAST_DELEGATE(FOnHeistBriefingContextReady);

/**
 * 플레이어별 브리핑 입력 진입점.
 *
 * 브리핑 UI와 브리핑 보드 사이의 얇은 브리지 역할을 맡는다.
 * 실제 authoritative drawing history 저장은 보드/SyncComponent가 담당한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistBriefingPlayerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeistBriefingPlayerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeBriefingContext(
		AHeistBriefingDrawingBoard* InDrawingBoard,
		EHeistBriefingViewMode InViewMode);

	UFUNCTION(BlueprintPure)
	AHeistBriefingDrawingBoard* GetDrawingBoard() const { return DrawingBoard; }

	UFUNCTION(BlueprintPure)
	EHeistBriefingViewMode GetViewMode() const { return ViewMode; }

	UFUNCTION(Server, Reliable)
	void ServerSetThiefSpawnPoint(FName InKey);

	UFUNCTION(Server, Reliable)
	void ServerSetPoliceObjectiveKey(FName InKey);

	UFUNCTION(Server, Unreliable)
	void ServerPushPreviewChunk(const FHeistBriefingStrokePreviewChunk& InChunk);

	UFUNCTION(Server, Reliable)
	void ServerCommitStroke(const FHeistBriefingStroke& InStroke);

	/** DrawingBoard 복제 완료(클라이언트) 또는 서버 세팅 완료 시 발동. */
	FOnHeistBriefingContextReady OnBriefingContextReady;

	/** BriefingPhaseComponent 직접 참조. BindPlayersToBriefingActors에서 주입. 서버 전용. */
	void SetBriefingPhase(UHeistBriefingPhaseComponent* InPhase);

private:
	UFUNCTION()
	void OnRep_DrawingBoard();

	UPROPERTY(ReplicatedUsing=OnRep_DrawingBoard)
	TObjectPtr<AHeistBriefingDrawingBoard> DrawingBoard;

	UPROPERTY(Replicated)
	EHeistBriefingViewMode ViewMode = EHeistBriefingViewMode::Thief;

	TWeakObjectPtr<UHeistBriefingPhaseComponent> BriefingPhase;
};
