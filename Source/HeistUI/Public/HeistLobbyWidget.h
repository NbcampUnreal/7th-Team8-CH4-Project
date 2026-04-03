#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistLobbyWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UMultiplayerSessionsSubsystem;

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

public:
	UFUNCTION(BlueprintCallable)
	void Setup();

protected:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlockInviteCode;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBoxPlayers;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonStartGame;

	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> SessionsSubsystem;

	FHeistMessageListenerHandle PlayersChangedListenerHandle;

	void OnPlayersChangedMessageReceived(FGameplayTag Channel, const struct FHeistLobbyPlayersChangedMessage& Message);

	void RefreshPlayerList(const TArray<FString>& PlayerNames);

	UFUNCTION()
	void OnButtonStartGameClicked();
};
