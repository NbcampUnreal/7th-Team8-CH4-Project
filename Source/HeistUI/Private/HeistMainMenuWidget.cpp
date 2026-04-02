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
		SetButtonsEnabled(true);
		return;
	}

	const FOnlineSessionSearchResult* Match = FindSessionByInviteCode(Results, PendingInviteCode);
	if (!Match)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HeistMainMenuWidget] No session found for invite code: %s"), *PendingInviteCode);
		SetButtonsEnabled(true);
		return;
	}

	SessionsSubsystem->JoinSession(*Match);
}

void UHeistMainMenuWidget::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
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

	PendingInviteCode = TextBoxInviteCode->GetText().ToString().TrimStartAndEnd();
	if (PendingInviteCode.Len() != InviteCodeLength)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HeistMainMenuWidget] Invalid invite code length: %d (expected %d)"),
			PendingInviteCode.Len(), InviteCodeLength);
		return;
	}

	SetButtonsEnabled(false);
	SessionsSubsystem->FindSessions(SessionConfig.MaxSearchResults);
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
