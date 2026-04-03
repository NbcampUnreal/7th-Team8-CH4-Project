#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistSystemMenuWidget.generated.h"

class UButton;
class UMultiplayerSessionsSubsystem;
struct FHeistSystemMenuToggleMessage;

/**
 * 시스템 메뉴 UI.
 * Message.UI.SystemMenuToggle 메시지를 구독해 토글된다.
 * UMG Blueprint에서 다음 위젯을 바인딩해야 한다:
 *   ButtonResume, ButtonSettings, ButtonLeave, ButtonQuit
 */
UCLASS()
class HEISTUI_API UHeistSystemMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnSettingsRequested();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonResume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonSettings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonLeave;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonQuit;

	UPROPERTY(EditDefaultsOnly, Category = "SystemMenu")
	FString MainMenuPath = TEXT("/Game/Maps/MainMenu");

	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> SessionsSubsystem;

	FHeistMessageListenerHandle ToggleListenerHandle;

	void OnToggleMessageReceived(FGameplayTag Channel, const FHeistSystemMenuToggleMessage& Message);

	void SetMenuVisibility(bool bVisible);

	UFUNCTION()
	void OnButtonResumeClicked();

	UFUNCTION()
	void OnButtonSettingsClicked();

	UFUNCTION()
	void OnButtonLeaveClicked();

	UFUNCTION()
	void OnButtonQuitClicked();

	UFUNCTION()
	void OnDestroySessionComplete(bool bWasSuccessful);
};
