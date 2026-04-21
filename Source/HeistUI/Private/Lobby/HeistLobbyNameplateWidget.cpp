#include "Lobby/HeistLobbyNameplateWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UHeistLobbyNameplateWidget::UpdateNameplate(const FString& PlayerName, bool bIsReady, bool bIsHost)
{
	if (TextBlockNickname)
	{
		TextBlockNickname->SetText(FText::FromString(PlayerName));
	}

	if (ImageReady)
	{
		ImageReady->SetVisibility(bIsReady ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UHeistLobbyNameplateWidget::SetTalkingState(bool bTalking)
{
	if (TextBlockNickname)
	{
		TextBlockNickname->SetColorAndOpacity(bTalking ? FLinearColor(0.2f, 1.0f, 0.2f, 1.0f) : FLinearColor::White);
	}
}
