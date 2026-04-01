#include "QuickMatchWidget.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Components/Button.h"

bool UQuickMatchWidget::Initialize()
{
	if (!Super::Initialize()) return false;

	if (ButtonPlay)
	{
		ButtonPlay->OnClicked.AddDynamic(this, &ThisClass::ButtonPlayClicked);
	}

	return true;
}

void UQuickMatchWidget::OnCreateSessionComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		ButtonPlay->SetIsEnabled(true);
	}
}

void UQuickMatchWidget::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful)
{
	if (!SessionsSubsystem) return;

	if (const FOnlineSessionSearchResult* Match = FindMatchingSession(Results))
	{
		SessionsSubsystem->JoinSession(*Match);
		return;
	}

	// No matching session found — become the host
	SessionsSubsystem->CreateSession(SessionConfig.NumPublicConnections, SessionConfig.MatchType);
}

void UQuickMatchWidget::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		ButtonPlay->SetIsEnabled(true);
	}
}

void UQuickMatchWidget::ButtonPlayClicked()
{
	ButtonPlay->SetIsEnabled(false);
	if (SessionsSubsystem)
	{
		SessionsSubsystem->FindSessions(SessionConfig.MaxSearchResults);
	}
}
