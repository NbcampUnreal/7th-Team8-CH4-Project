#pragma once

#include "CoreMinimal.h"
#include "MultiplayerWidgetBase.h"
#include "QuickMatchWidget.generated.h"

class UButton;

/**
 * Widget for automatic matchmaking with a single Play button.
 * Flow: FindSessions → join if match found, create if not.
 * Travel logic is handled externally by MultiplayerGameInstance or game-side delegate binding.
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UQuickMatchWidget : public UMultiplayerWidgetBase
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;

	virtual void OnCreateSessionComplete(bool bWasSuccessful) override;
	virtual void OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& Results, bool bWasSuccessful) override;
	virtual void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result) override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonPlay;

	UFUNCTION()
	void ButtonPlayClicked();
};
