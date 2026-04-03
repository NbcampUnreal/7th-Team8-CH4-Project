#pragma once

#include "CoreMinimal.h"
#include "MultiplayerWidgetBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "HeistMainMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

/**
 * 메인 메뉴 UI. 세션 생성 및 초대 코드 기반 참가를 제공한다.
 * UMG Blueprint에서 다음 위젯을 바인딩해야 한다:
 *   ButtonHost, ButtonJoinByCode, TextBoxInviteCode, TextBlockInviteCode
 */
UCLASS()
class HEISTUI_API UHeistMainMenuWidget : public UMultiplayerWidgetBase
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;

	virtual void OnCreateSessionComplete(bool bWasSuccessful) override;
	virtual void OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful) override;
	virtual void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result) override;

private:
	// 세션 생성 버튼 (호스트)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonHost;

	// 초대 코드로 참가 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonJoinByCode;

	// 초대 코드 입력 필드 (6자)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> TextBoxInviteCode;

	// 상태/오류 메시지 표시 (선택적 바인딩)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlockStatus;

	UFUNCTION()
	void ButtonHostClicked();

	UFUNCTION()
	void ButtonJoinByCodeClicked();

	void SetButtonsEnabled(bool bEnabled);
	void ShowStatus(const FString& Message);
	void ClearStatus();

	FTimerHandle StatusClearTimerHandle;
	static constexpr float StatusDisplaySeconds = 3.0f;

	// FindSessions 비동기 대기 중 코드가 변경되는 것을 방지하기 위해 버튼 클릭 시점에 저장
	FString PendingInviteCode;

	static const int32 InviteCodeLength;
};
