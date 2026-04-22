#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistPlayHUD.generated.h"

class UProgressBar;
class UTextBlock;
class UOverlay;
class UWidgetSwitcher;
class UVerticalBox;

/*
* 플레이 화면에 띄울 가장 상위 계층의 WBP
* 도둑 정보는 ThiefSlotWidget이 담당
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
	void InitializeIfBriefingPhase();
	void UpdateTimerText(float RemainingTime);

	UPROPERTY()
	TObjectPtr<UVerticalBox> VBox_ThiefStates;

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

	FTimerHandle CheckTimerHandle;
	FHeistMessageListenerHandle PhaseTimeHandle;
	FHeistMessageListenerHandle ZoneScoresHandle;
	FHeistMessageListenerHandle PoliceObjectiveHandle;
};
