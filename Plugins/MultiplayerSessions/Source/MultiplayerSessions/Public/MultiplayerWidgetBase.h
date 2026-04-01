#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionTypes.h"
#include "MultiplayerWidgetBase.generated.h"

class UMultiplayerSessionsSubsystem;

/**
 * Abstract base class for all multiplayer session widgets.
 * Handles subsystem binding/unbinding and common UI setup.
 * Subclasses implement session event responses.
 */
UCLASS(Abstract)
class MULTIPLAYERSESSIONS_API UMultiplayerWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void Setup(const FMultiplayerSessionConfig& Config);

protected:
	virtual void NativeDestruct() override;

	// Override these in subclasses to respond to session events
	virtual void OnCreateSessionComplete(bool bWasSuccessful) {}
	virtual void OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful) {}
	virtual void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result) {}

	// Removes widget and restores game input mode (call from button handlers when done)
	void SetGameInputMode();

	// Returns the first search result whose MatchType matches SessionConfig, or nullptr
	const FOnlineSessionSearchResult* FindMatchingSession(const TArray<FOnlineSessionSearchResult>& Results) const;

	// Returns the first search result whose InviteCode matches the given code, or nullptr
	const FOnlineSessionSearchResult* FindSessionByInviteCode(const TArray<FOnlineSessionSearchResult>& Results, const FString& InviteCode) const;

	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> SessionsSubsystem;

	FMultiplayerSessionConfig SessionConfig;

private:
	// Private UFUNCTION handlers dispatch to virtual methods (enables virtual dispatch with AddDynamic)
	UFUNCTION()
	void HandleCreateSessionComplete(bool bWasSuccessful);
	void HandleFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful);
	void HandleJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);

	void BindDelegates();
	void UnbindDelegates();
	void RestoreGameInputMode();
};
