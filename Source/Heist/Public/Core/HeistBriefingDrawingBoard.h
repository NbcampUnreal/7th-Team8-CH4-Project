
#pragma once

#include "CoreMinimal.h"
#include "Core/HeistMatchTypes.h"
#include "GameFramework/Actor.h"
#include "HeistBriefingDrawingBoard.generated.h"

class UHeistBriefingDrawingSyncComponent;

/**
 * 브리핑용 드로잉/플랜 보드.
 *
 * 이 액터는 브리핑 UI가 참조하는 네트워크 보드이며,
 * 실제 stroke 이력 저장은 UHeistBriefingDrawingSyncComponent에 위임한다.
 */
UCLASS()
class HEIST_API AHeistBriefingDrawingBoard : public AActor
{
	GENERATED_BODY()

public:
	AHeistBriefingDrawingBoard();
	
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	
	void BroadcastPreviewChunkAuthoritative(const FHeistBriefingStrokePreviewChunk& InChunk);
	void CommitStrokeAuthoritative(const FHeistBriefingStroke& InStroke);

	UFUNCTION(BlueprintPure)
	UHeistBriefingDrawingSyncComponent* GetDrawingSyncComponent() const { return DrawingSyncComponent; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UHeistBriefingDrawingSyncComponent> DrawingSyncComponent;
	
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastReceivePreviewChunk(const FHeistBriefingStrokePreviewChunk& InChunk);
};
