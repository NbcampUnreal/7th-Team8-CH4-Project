#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistVoiceEntryWidget.generated.h"

class UTextBlock;

UCLASS()
class HEISTUI_API UHeistVoiceEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateEntry(const FString& PlayerName, bool bTalking);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TalkingIndicatorText;
};
