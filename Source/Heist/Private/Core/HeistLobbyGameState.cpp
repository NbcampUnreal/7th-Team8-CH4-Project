#include "Core/HeistLobbyGameState.h"

#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "Net/UnrealNetwork.h"

void AHeistLobbyGameState::AddPlayer(const FString& PlayerName)
{
	LobbyPlayerNames.Add(PlayerName);
	BroadcastPlayersChanged();
}

void AHeistLobbyGameState::RemovePlayer(const FString& PlayerName)
{
	LobbyPlayerNames.Remove(PlayerName);
	BroadcastPlayersChanged();
}

void AHeistLobbyGameState::OnRep_LobbyPlayerNames()
{
	BroadcastPlayersChanged();
}

void AHeistLobbyGameState::BroadcastPlayersChanged()
{
	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);

	FHeistLobbyPlayersChangedMessage Message;
	Message.PlayerNames = LobbyPlayerNames;

	MessageSubsystem.BroadcastMessage(HeistMessageTags::Message_Lobby_PlayersChanged, Message);
}

void AHeistLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeistLobbyGameState, LobbyPlayerNames);
}
