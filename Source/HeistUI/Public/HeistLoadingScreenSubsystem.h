#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistLoadingScreenSubsystem.generated.h"

class UHeistLoadingScreenWidget;
struct FHeistTravelSeamlessStartMessage;
struct FHeistTravelSeamlessEndMessage;

/**
 * 레벨 전환 시 로딩 화면을 표시/숨김 처리하는 GameInstanceSubsystem.
 *
 * Heist 모듈에 의존하지 않음. HeistUI 모듈 독립.
 *
 * Seamless travel (Lobby → Game):
 *   Message_Travel_SeamlessStart → ShowLoadingScreen(bIsSeamless=true)
 *   Message_Travel_SeamlessEnd   → TriggerFillAnimation()
 *
 * Non-seamless travel (MainMenu → Lobby):
 *   PreLoadMap          → ShowLoadingScreen(bIsSeamless=false)
 *   PostLoadMapWithWorld → TriggerFillAnimation()
 *
 * 위젯 클래스는 DefaultGame.ini 에서 설정:
 * [/Script/HeistUI.HeistLoadingScreenSubsystem]
 * LoadingScreenClass=/Game/HeistUI/Loading/WBP_LoadingScreen.WBP_LoadingScreen_C
 */
UCLASS(Config = Game)
class HEISTUI_API UHeistLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	void OnSeamlessStart(FGameplayTag Channel, const FHeistTravelSeamlessStartMessage& Msg);
	void OnSeamlessEnd(FGameplayTag Channel, const FHeistTravelSeamlessEndMessage& Msg);

	void ShowLoadingScreen(bool bIsSeamless);
	void TriggerFillAnimation();

	UFUNCTION()
	void HideLoadingScreen();

	bool IsTransitionMap(UWorld* World) const;

	UPROPERTY(Config)
	TSoftClassPtr<UHeistLoadingScreenWidget> LoadingScreenClass;

	// TriggerFillAnimation 후 FallbackTimeout 초 내 OnFillComplete 미수신 시 강제 숨김.
	UPROPERTY(Config)
	float FallbackTimeout = 3.0f;

	UPROPERTY()
	TObjectPtr<UHeistLoadingScreenWidget> LoadingScreenInstance;

	bool bFillAnimationStarted = false;

	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;
	FHeistMessageListenerHandle SeamlessStartHandle;
	FHeistMessageListenerHandle SeamlessEndHandle;
	FTimerHandle FallbackTimerHandle;

	static constexpr int32 LoadingScreenZOrder = 100;
};
