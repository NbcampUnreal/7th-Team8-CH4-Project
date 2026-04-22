#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistPlayHUD.generated.h"

class UProgressBar;
class UTextBlock;
class UOverlay;
class UWidgetSwitcher;
class UHorizontalBox;
class UHeistThiefSlotSetWidget;
struct FHeistPlayHUDThiefStateChangedMessage;

/*
* 플레이 화면에 띄울 가장 상위 계층의 WBP
* 도둑 정보는 ThiefSlotWidget이 담당하며,
* VBox_ThiefStates에 자식으로 배치됨
*/
UCLASS()
class HEISTUI_API UHeistPlayHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable)
	void InitializeThiefSlots();

protected:
	void RefreshThiefSlots();
	void HandleThiefStateChanged(const FHeistPlayHUDThiefStateChangedMessage& Message);

	void InitializeIfBriefingPhase();
	void UpdateTimerText(float RemainingTime);

	// 도둑 상태 슬롯 컨테이너
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HBox_StatusGroup;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Thief_Police_Switcher;

	UPROPERTY()
	TObjectPtr<UTextBlock> PlayTimerText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ProtectObjective;

	UPROPERTY()
	TArray<TObjectPtr<UProgressBar>> QuotaBars;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> Overlay_Thief_Only;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> Overlay_Police_Only;

	// 플레이 화면에 표시 중인 도둑 슬롯들
	UPROPERTY()
	TArray<TObjectPtr<UHeistThiefSlotSetWidget>> ThiefSlots;

	UPROPERTY()
	TMap<FString, TObjectPtr<UHeistThiefSlotSetWidget>> ThiefSlotsByPlayerName;

	FTimerHandle CheckTimerHandle;
	FHeistMessageListenerHandle PhaseTimeHandle;
	FHeistMessageListenerHandle ZoneScoresHandle;
	FHeistMessageListenerHandle PoliceObjectiveHandle;
	FHeistMessageListenerHandle PlayersChangedHandle;
	FHeistMessageListenerHandle ThiefStateChangedHandle;
};
