#pragma once

#include "CoreMinimal.h"
#include "MultiplayerWidgetBase.h"
#include "SessionBrowser.generated.h"

class UButton;

/**
 * Widget for manual session management with explicit Host and Join buttons.
 * Suitable for custom lobby rooms.
 * Travel logic is handled externally by MultiplayerGameInstance or game-side delegate binding.
 */
UCLASS()
class MULTIPLAYERSESSIONS_API USessionBrowser : public UMultiplayerWidgetBase
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;

	virtual void OnCreateSessionComplete(bool bWasSuccessful) override;
	virtual void OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful) override;
	virtual void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result) override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonHost;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonJoin;

	UFUNCTION()
	void ButtonHostClicked();

	UFUNCTION()
	void ButtonJoinClicked();
};
