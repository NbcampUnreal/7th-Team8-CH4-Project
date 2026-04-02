#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
	// 초대 코드 표시
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlockInviteCode;

	// 플레이어 목록 (동적으로 TextBlock 추가)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBoxPlayers;

	// 게임 시작 버튼 (호스트 전용 — BP에서 호스트만 보이도록 처리)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonStartGame;

	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> SessionsSubsystem;

	// GameState 델리게이트 핸들
	FDelegateHandle LobbyPlayersChangedHandle;

	UFUNCTION()
	void ButtonStartGameClicked();

	void RefreshPlayerList();
};
