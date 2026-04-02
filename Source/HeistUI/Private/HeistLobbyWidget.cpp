#include "HeistLobbyWidget.h"

#include "Core/HeistLobbyGameMode.h"
#include "Core/HeistLobbyGameState.h"
#include "MultiplayerSessionsSubsystem.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

bool UHeistLobbyWidget::Initialize()
{
	if (!Super::Initialize()) return false;

	if (ButtonStartGame)
	{
		ButtonStartGame->OnClicked.AddDynamic(this, &ThisClass::ButtonStartGameClicked);
	}

	return true;
}

void UHeistLobbyWidget::NativeDestruct()
{
	AHeistLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AHeistLobbyGameState>();
	if (IsValid(LobbyGameState))
	{
		LobbyGameState->OnLobbyPlayersChanged.Remove(LobbyPlayersChangedHandle);
	}

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

	AHeistLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AHeistLobbyGameState>();
	if (IsValid(LobbyGameState))
	{
		LobbyPlayersChangedHandle = LobbyGameState->OnLobbyPlayersChanged.AddUObject(this, &ThisClass::RefreshPlayerList);
		RefreshPlayerList();
	}
}

void UHeistLobbyWidget::RefreshPlayerList()
{
	if (!ScrollBoxPlayers) return;

	ScrollBoxPlayers->ClearChildren();

	AHeistLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AHeistLobbyGameState>();
	if (!IsValid(LobbyGameState)) return;

	for (const FString& PlayerName : LobbyGameState->GetLobbyPlayerNames())
	{
		UTextBlock* PlayerEntry = NewObject<UTextBlock>(this);
		PlayerEntry->SetText(FText::FromString(PlayerName));
		ScrollBoxPlayers->AddChild(PlayerEntry);
	}
}

void UHeistLobbyWidget::ButtonStartGameClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!IsValid(PC)) return;

	if (AHeistLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AHeistLobbyGameMode>())
	{
		LobbyGameMode->RequestStartGame(PC);
	}
}
