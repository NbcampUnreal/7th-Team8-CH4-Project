#include "Core/HeistLobbyGameMode.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

AHeistLobbyGameMode::AHeistLobbyGameMode()
{
	GameMapPath = TEXT("/Game/Maps/Game");
	MinPlayersToStart = 5;
}

void AHeistLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!IsValid(NewPlayer)) return;

	const APlayerState* PlayerState = NewPlayer->GetPlayerState<APlayerState>();
	const FString PlayerName = IsValid(PlayerState) ? PlayerState->GetPlayerName() : TEXT("Unknown");
	UE_LOG(LogTemp, Log, TEXT("[HeistLobbyGameMode] PostLogin: %s (Total: %d)"),
		*PlayerName, GameState->PlayerArray.Num());
}

void AHeistLobbyGameMode::Logout(AController* Exiting)
{
	if (IsValid(Exiting))
	{
		const APlayerState* PlayerState = Exiting->GetPlayerState<APlayerState>();
		const FString PlayerName = IsValid(PlayerState) ? PlayerState->GetPlayerName() : TEXT("Unknown");
		UE_LOG(LogTemp, Log, TEXT("[HeistLobbyGameMode] Logout: %s"), *PlayerName);
	}

	Super::Logout(Exiting);
}

void AHeistLobbyGameMode::RequestStartGame(APlayerController* Requester)
{
	if (!IsValid(Requester)) return;

	if (!IsHostController(Requester))
	{
		UE_LOG(LogTemp, Warning, TEXT("[HeistLobbyGameMode] RequestStartGame: Non-host request ignored."));
		return;
	}

	if (!IsValid(GameState)) return;

	const int32 CurrentPlayers = GameState->PlayerArray.Num();
	if (CurrentPlayers < MinPlayersToStart)
	{
		UE_LOG(LogTemp, Log, TEXT("[HeistLobbyGameMode] RequestStartGame: Not enough players (%d/%d)."),
			CurrentPlayers, MinPlayersToStart);
		return;
	}

	ensureAlwaysMsgf(!GameMapPath.IsEmpty(), TEXT("AHeistLobbyGameMode: GameMapPath is not set."));

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	UE_LOG(LogTemp, Log, TEXT("[HeistLobbyGameMode] Starting game. Traveling to: %s"), *GameMapPath);
	World->ServerTravel(GameMapPath);
}

bool AHeistLobbyGameMode::IsHostController(APlayerController* PlayerController) const
{
	if (!IsValid(GameState)) return false;
	if (GameState->PlayerArray.IsEmpty()) return false;

	const APlayerState* HostPlayerState = GameState->PlayerArray[0];
	if (!IsValid(HostPlayerState)) return false;

	return HostPlayerState->GetOwner() == PlayerController;
}
