#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistLobbyNameplateWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class HEISTUI_API UHeistLobbyNameplateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateNameplate(const FString& PlayerName, bool bIsReady, bool bIsHost);
	void SetTalkingState(bool bTalking);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlockNickname;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ImageReady;
};
