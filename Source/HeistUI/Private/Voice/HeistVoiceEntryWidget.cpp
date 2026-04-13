#include "Voice/HeistVoiceEntryWidget.h"

#include "Components/TextBlock.h"

void UHeistVoiceEntryWidget::UpdateEntry(const FString& PlayerName, bool bTalking)
{
	PlayerNameText->SetText(FText::FromString(PlayerName));

	if (bTalking)
	{
		PlayerNameText->SetColorAndOpacity(FLinearColor(0.2f, 1.0f, 0.2f, 1.0f));
		TalkingIndicatorText->SetText(FText::FromString(TEXT("●")));
		TalkingIndicatorText->SetColorAndOpacity(FLinearColor(0.2f, 1.0f, 0.2f, 1.0f));
	}
	else
	{
		PlayerNameText->SetColorAndOpacity(FLinearColor::White);
		TalkingIndicatorText->SetText(FText::GetEmpty());
	}
}
