#include "MultiplayerWidgetBase.h"
#include "MultiplayerSessionsSubsystem.h"
#include "MultiplayerSessionTypes.h"
#include "OnlineSessionSettings.h"

void UMultiplayerWidgetBase::Setup(const FMultiplayerSessionConfig& Config)
{
	SessionConfig = Config;

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);
		}
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		SessionsSubsystem = GI->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	BindDelegates();
}

void UMultiplayerWidgetBase::NativeDestruct()
{
	RestoreGameInputMode();
	UnbindDelegates();
	Super::NativeDestruct();
}

void UMultiplayerWidgetBase::SetGameInputMode()
{
	RemoveFromParent();
	RestoreGameInputMode();
}

void UMultiplayerWidgetBase::RestoreGameInputMode()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->SetInputMode(FInputModeGameOnly{});
			PC->SetShowMouseCursor(false);
		}
	}
}

const FOnlineSessionSearchResult* UMultiplayerWidgetBase::FindMatchingSession(const TArray<FOnlineSessionSearchResult>& Results) const
{
	for (const FOnlineSessionSearchResult& Result : Results)
	{
		FString SettingsValue;
		Result.Session.SessionSettings.Get(MultiplayerSessionConstants::MatchTypeKey, SettingsValue);
		if (SettingsValue == SessionConfig.MatchType)
		{
			return &Result;
		}
	}
	return nullptr;
}

void UMultiplayerWidgetBase::BindDelegates()
{
	if (!SessionsSubsystem) return;

	SessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::HandleCreateSessionComplete);
	SessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::HandleFindSessionsComplete);
	SessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::HandleJoinSessionComplete);
}

void UMultiplayerWidgetBase::UnbindDelegates()
{
	if (!SessionsSubsystem) return;

	SessionsSubsystem->MultiplayerOnCreateSessionComplete.RemoveDynamic(this, &ThisClass::HandleCreateSessionComplete);
	SessionsSubsystem->MultiplayerOnFindSessionsComplete.RemoveAll(this);
	SessionsSubsystem->MultiplayerOnJoinSessionComplete.RemoveAll(this);
}

void UMultiplayerWidgetBase::HandleCreateSessionComplete(bool bWasSuccessful)
{
	OnCreateSessionComplete(bWasSuccessful);
}

void UMultiplayerWidgetBase::HandleFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful)
{
	OnFindSessionsComplete(Results, bWasSuccessful);
}

void UMultiplayerWidgetBase::HandleJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	OnJoinSessionComplete(Result);
}
