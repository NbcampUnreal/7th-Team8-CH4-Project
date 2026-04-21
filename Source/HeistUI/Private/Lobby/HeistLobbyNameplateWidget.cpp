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
