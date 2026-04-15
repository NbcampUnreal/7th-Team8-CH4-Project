
#pragma once

#include "CoreMinimal.h"
#include "Core/HeistMatchTypes.h"
#include "GameFramework/Actor.h"
#include "HeistBriefingVoiceAnchor.generated.h"

/**
 * 현재 미사용 상태.
 * 브리핑 보이스를 Pawn Root 기준으로 유지하기로 하면서 분리 앵커 연출은 보류됐다.
 * 추후 브리핑 전용 보이스 위치 연출이 필요해지면 다시 연결한다.
 */
UCLASS()
class HEIST_API AHeistBriefingVoiceAnchor : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> VoiceAttachRoot;
	
	UPROPERTY(EditDefaultsOnly)
	EHeistTeam SupportedTeam = EHeistTeam::None;
	
public:
	AHeistBriefingVoiceAnchor();
	
	UFUNCTION(BlueprintPure)
	USceneComponent* GetVoiceAttachRoot() const { return VoiceAttachRoot; }
};
