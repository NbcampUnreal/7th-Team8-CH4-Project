#pragma once

#include "CoreMinimal.h"
#include "MultiplayerGameInstance.h"
#include "HeistGameInstance.generated.h"

/**
 * Heist 프로젝트 전용 GameInstance.
 * Listen-server 로비 모드를 기본값으로 사용한다.
 * 데디케이티드 서버 전환 시 RequestDedicatedServerAddress()를 오버라이드한다.
 */
UCLASS()
class UHeistGameInstance : public UMultiplayerGameInstance
{
	GENERATED_BODY()

public:
	UHeistGameInstance();

protected:
	// TODO: 데디케이티드 서버 주소 확정 후 실제 로직 구현
	virtual void RequestDedicatedServerAddress(EMultiplayerTravelPurpose Purpose) override;
};
