#include "Core/HeistLobbyGameState.h"

#include "Net/UnrealNetwork.h"

void AHeistLobbyGameState::AddPlayer(const FString& PlayerName)
{
	LobbyPlayerNames.Add(PlayerName);
	OnLobbyPlayersChanged.Broadcast();
}

void AHeistLobbyGameState::RemovePlayer(const FString& PlayerName)
{
	LobbyPlayerNames.Remove(PlayerName);
	OnLobbyPlayersChanged.Broadcast();
}

void AHeistLobbyGameState::OnRep_LobbyPlayerNames()
{
	OnLobbyPlayersChanged.Broadcast();
}

void AHeistLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeistLobbyGameState, LobbyPlayerNames);
}
