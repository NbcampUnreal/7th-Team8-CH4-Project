#include "SessionBrowser.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Components/Button.h"

bool USessionBrowser::Initialize()
{
	if (!Super::Initialize()) return false;

	if (ButtonHost)
	{
		ButtonHost->OnClicked.AddDynamic(this, &ThisClass::ButtonHostClicked);
	}
	if (ButtonJoin)
	{
		ButtonJoin->OnClicked.AddDynamic(this, &ThisClass::ButtonJoinClicked);
	}

	return true;
}

void USessionBrowser::OnCreateSessionComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		ButtonHost->SetIsEnabled(true);
	}
}

void USessionBrowser::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful)
{
	if (!SessionsSubsystem) return;

	if (const FOnlineSessionSearchResult* Match = FindMatchingSession(Results))
	{
		SessionsSubsystem->JoinSession(*Match);
		return;
	}

	ButtonJoin->SetIsEnabled(true);
}

void USessionBrowser::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		ButtonJoin->SetIsEnabled(true);
	}
}

void USessionBrowser::ButtonHostClicked()
{
	ButtonHost->SetIsEnabled(false);
	if (SessionsSubsystem)
	{
		SessionsSubsystem->CreateSession(SessionConfig.NumPublicConnections, SessionConfig.MatchType);
	}
}

void USessionBrowser::ButtonJoinClicked()
{
	ButtonJoin->SetIsEnabled(false);
	if (SessionsSubsystem)
	{
		SessionsSubsystem->FindSessions(SessionConfig.MaxSearchResults);
	}
}
