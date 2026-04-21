#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistPlayUISubsystem.generated.h"

class UHeistPlayHUD;

/**
 * 실게임 중 플레이 화면 HUD(도둑 정보, 경찰 정보)를 관리.
 * Execution Phase 진입 시 위젯 생성 후 뷰포트 추가.
 */
UCLASS()
class HEISTUI_API UHeistPlayUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void ShowPlayHUD(TSubclassOf<UObject> InWidgetClass);
	void HidePlayHUD();

private:
	UPROPERTY()
	TObjectPtr<UHeistPlayHUD> PlayHUDInstance;

	FHeistMessageListenerHandle ReadyPhaseHandle;
	FHeistMessageListenerHandle EndHandle;
};
