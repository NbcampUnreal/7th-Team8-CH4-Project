#include "HeistLobbyWidget.h"

#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "Core/HeistLobbyGameMode.h"

#include "MultiplayerSessionsSubsystem.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

bool UHeistLobbyWidget::Initialize()
{
	if (!Super::Initialize()) return false;

	if (IsDesignTime()) return true;

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);

	PlayersChangedListenerHandle = MessageSubsystem.RegisterListener<FHeistLobbyPlayersChangedMessage>(
		HeistMessageTags::Message_Lobby_PlayersChanged,
		[this](FGameplayTag Channel, const FHeistLobbyPlayersChangedMessage& Message)
		{
			OnPlayersChangedMessageReceived(Channel, Message);
		});

	if (ButtonStartGame)
	{
		ButtonStartGame->OnClicked.AddDynamic(this, &ThisClass::OnButtonStartGameClicked);
	}

	return true;
}

void UHeistLobbyWidget::NativeDestruct()
{
	PlayersChangedListenerHandle.Unregister();

	Super::NativeDestruct();
}

void UHeistLobbyWidget::Setup()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		SessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (IsValid(SessionsSubsystem))
	{
		const FString InviteCode = SessionsSubsystem->GetLastCreatedSessionInviteCode();
		if (TextBlockInviteCode)
		{
			TextBlockInviteCode->SetText(FText::FromString(InviteCode));
		}
	}
}

void UHeistLobbyWidget::OnPlayersChangedMessageReceived(FGameplayTag Channel, const FHeistLobbyPlayersChangedMessage& Message)
{
	RefreshPlayerList(Message.PlayerNames);
}

void UHeistLobbyWidget::RefreshPlayerList(const TArray<FString>& PlayerNames)
{
	if (!ScrollBoxPlayers) return;

	ScrollBoxPlayers->ClearChildren();

	for (const FString& PlayerName : PlayerNames)
	{
		UTextBlock* PlayerEntry = NewObject<UTextBlock>(this);
		PlayerEntry->SetText(FText::FromString(PlayerName));
		ScrollBoxPlayers->AddChild(PlayerEntry);
	}
}

void UHeistLobbyWidget::OnButtonStartGameClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!IsValid(PC)) return;

	if (AHeistLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AHeistLobbyGameMode>())
	{
		LobbyGameMode->RequestStartGame(PC);
	}
}
