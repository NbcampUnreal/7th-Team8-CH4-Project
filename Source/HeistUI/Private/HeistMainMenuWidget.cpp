#include "HeistMainMenuWidget.h"

#include "MultiplayerSessionsSubsystem.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

const int32 UHeistMainMenuWidget::InviteCodeLength = 6;

bool UHeistMainMenuWidget::Initialize()
{
	if (!Super::Initialize()) return false;

	if (ButtonHost)
	{
		ButtonHost->OnClicked.AddDynamic(this, &ThisClass::ButtonHostClicked);
	}
	if (ButtonJoinByCode)
	{
		ButtonJoinByCode->OnClicked.AddDynamic(this, &ThisClass::ButtonJoinByCodeClicked);
	}
	return true;
}

void UHeistMainMenuWidget::OnCreateSessionComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		ShowStatus(TEXT("세션 생성에 실패했습니다."));
		SetButtonsEnabled(true);
		return;
	}

	// 세션 생성 성공 시 HeistGameInstance::HandleSessionCreated 가 로비 맵으로 ServerTravel 한다.
}

void UHeistMainMenuWidget::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful)
{
	if (!IsValid(SessionsSubsystem)) return;

	if (!bWasSuccessful)
	{
		ShowStatus(TEXT("세션을 찾을 수 없습니다."));
		SetButtonsEnabled(true);
		return;
	}

	const FOnlineSessionSearchResult* Match = FindSessionByInviteCode(Results, PendingInviteCode);
	if (!Match)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HeistMainMenuWidget] No session found for invite code: %s"), *PendingInviteCode);
		ShowStatus(TEXT("초대 코드가 올바르지 않습니다."));
		SetButtonsEnabled(true);
		return;
	}

	SessionsSubsystem->JoinSession(*Match);
}

void UHeistMainMenuWidget::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		ShowStatus(TEXT("세션 참가에 실패했습니다."));
		SetButtonsEnabled(true);
	}
	// 성공 시: HeistGameInstance::HandleSessionJoined 가 ClientTravel을 처리한다.
}

void UHeistMainMenuWidget::ButtonHostClicked()
{
	if (!IsValid(SessionsSubsystem)) return;

	SetButtonsEnabled(false);
	SessionsSubsystem->CreateSession(SessionConfig.NumPublicConnections, SessionConfig.MatchType);
}

void UHeistMainMenuWidget::ButtonJoinByCodeClicked()
{
	if (!IsValid(SessionsSubsystem)) return;
	if (!TextBoxInviteCode) return;

	PendingInviteCode = TextBoxInviteCode->GetText().ToString().TrimStartAndEnd().ToUpper();
	if (PendingInviteCode.Len() != InviteCodeLength)
	{
		ShowStatus(TEXT("초대 코드는 6자리입니다."));
		return;
	}

	for (TCHAR Ch : PendingInviteCode)
	{
		if (!FChar::IsAlpha(Ch) && !FChar::IsDigit(Ch))
		{
			ShowStatus(TEXT("초대 코드는 영문자와 숫자만 사용 가능합니다."));
			return;
		}
	}

	SetButtonsEnabled(false);
	SessionsSubsystem->FindSessions(SessionConfig.MaxSearchResults);
}

void UHeistMainMenuWidget::ShowStatus(const FString& Message)
{
	if (!TextBlockStatus) return;

	TextBlockStatus->SetText(FText::FromString(Message));

	GetWorld()->GetTimerManager().SetTimer(
		StatusClearTimerHandle,
		this, &ThisClass::ClearStatus,
		StatusDisplaySeconds,
		false
	);
}

void UHeistMainMenuWidget::ClearStatus()
{
	if (TextBlockStatus)
	{
		TextBlockStatus->SetText(FText::GetEmpty());
	}
}

void UHeistMainMenuWidget::SetButtonsEnabled(bool bEnabled)
{
	if (ButtonHost)
	{
		ButtonHost->SetIsEnabled(bEnabled);
	}
	if (ButtonJoinByCode)
	{
		ButtonJoinByCode->SetIsEnabled(bEnabled);
	}
}
