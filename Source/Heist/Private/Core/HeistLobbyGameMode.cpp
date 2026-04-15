#include "Core/HeistLobbyGameMode.h"

#include "Core/HeistLobbyGameState.h"
#include "Core/HeistPlayerState.h"
#include "MultiplayerSessionsSubsystem.h"

AHeistLobbyGameMode::AHeistLobbyGameMode()
{
	GameStateClass = AHeistLobbyGameState::StaticClass();
	GameMapPath = TEXT("/Game/Project_Heist/Maps/NewToyMuseum?listen");
	MinPlayersToStart = 5;
	bUseSeamlessTravel = true;
}

void AHeistLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance)) return;

	UMultiplayerSessionsSubsystem* SessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	if (!IsValid(SessionsSubsystem)) return;

	const FString InviteCode = SessionsSubsystem->GetLastCreatedSessionInviteCode();
	if (InviteCode.IsEmpty()) return;

	AHeistLobbyGameState* LobbyGameState = GetGameState<AHeistLobbyGameState>();
	if (IsValid(LobbyGameState))
	{
		LobbyGameState->SetInviteCode(InviteCode);
	}
}

void AHeistLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!IsValid(NewPlayer)) return;

	AHeistPlayerState* PlayerState = NewPlayer->GetPlayerState<AHeistPlayerState>();
	const FString PlayerName = IsValid(PlayerState) ? PlayerState->GetPlayerName() : TEXT("Unknown");

	if (IsHostController(NewPlayer) && IsValid(PlayerState))
	{
		PlayerState->SetIsHost(true);
		PlayerState->SetIsReady(true);
	}

	UE_LOG(LogTemp, Log, TEXT("[HeistLobbyGameMode] PostLogin: %s (Total: %d)"),
		*PlayerName, GameState->PlayerArray.Num());
}

void AHeistLobbyGameMode::Logout(AController* Exiting)
{
	UWorld* World = GetWorld();
	// PIE 강제 종료 시 World teardown 중에 Logout이 호출될 수 있으므로 처리를 건너뜀
	if (IsValid(Exiting) && IsValid(World) && !World->bIsTearingDown)
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

	AHeistLobbyGameState* LobbyGameState = GetGameState<AHeistLobbyGameState>();
	if (!IsValid(LobbyGameState) || !LobbyGameState->AreAllPlayersReady())
	{
		UE_LOG(LogTemp, Log, TEXT("[HeistLobbyGameMode] RequestStartGame: Not all players are ready."));
		return;
	}

	ensureAlwaysMsgf(!GameMapPath.IsEmpty(), TEXT("AHeistLobbyGameMode: GameMapPath is not set."));

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (IsValid(PC))
		{
			PC->StopTalking();
		}
	}

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
