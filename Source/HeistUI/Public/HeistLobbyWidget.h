#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistLobbyWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class AHeistPlayerController;
class AHeistLobbyGameState;

/**
 * 로비 맵 UI.
 * 초대 코드 표시, 플레이어 목록, 호스트 전용 게임 시작 버튼을 제공한다.
 * UMG Blueprint에서 다음 위젯을 바인딩해야 한다:
 *   TextBlockInviteCode, ScrollBoxPlayers, ButtonStartGame
 */
UCLASS()
class HEISTUI_API UHeistLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlockInviteCode;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBoxPlayers;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonStartGame;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ButtonReady;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ButtonCopyInviteCode;

	FHeistMessageListenerHandle PlayersChangedListenerHandle;
	FHeistMessageListenerHandle ReadyStateChangedListenerHandle;
	FHeistMessageListenerHandle InviteCodeChangedListenerHandle;

	void OnPlayersChangedMessageReceived(FGameplayTag Channel, const struct FHeistLobbyPlayersChangedMessage& Message);
	void OnReadyStateChangedMessageReceived(FGameplayTag Channel, const struct FHeistLobbyReadyStateChangedMessage& Message);
	void OnInviteCodeChangedMessageReceived(FGameplayTag Channel, const struct FHeistLobbyInviteCodeChangedMessage& Message);

	FString CurrentInviteCode;

	void RefreshPlayerList();
	void RefreshStartButtonState();
	void RefreshInviteCode(const FString& NewInviteCode);

	UFUNCTION()
	void OnButtonStartGameClicked();

	UFUNCTION()
	void OnButtonReadyClicked();

	UFUNCTION()
	void OnButtonCopyInviteCodeClicked();
};
