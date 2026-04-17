#include "Core/HeistLobbyGameMode.h"

#include "Core/HeistLobbyGameState.h"
#include "Core/HeistPlayerController.h"
#include "Core/HeistPlayerState.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Voice/HeistVoiceSubsystem.h"

#include "GameFramework/GameStateBase.h"

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

	if (bStartGameRequested)
	{
		UE_LOG(LogTemp, Log, TEXT("[HeistLobbyGameMode] RequestStartGame: already processing travel."));
		return;
	}

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

	bStartGameRequested = true;
	ExpectedPlayersForMatchTravel.Reset();
	PlayersReadyForMatchTravel.Reset();

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AHeistPlayerState* HeistPlayerState = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPlayerState))
		{
			continue;
		}

		AHeistPlayerController* HeistPC = Cast<AHeistPlayerController>(HeistPlayerState->GetOwner());
		if (!IsValid(HeistPC))
		{
			continue;
		}

		ExpectedPlayersForMatchTravel.Add(HeistPlayerState);
		HeistPC->ClientPrepareForMatchTravel();
	}

	World->GetTimerManager().SetTimer(
		MatchTravelReadyTimeoutHandle,
		this,
		&AHeistLobbyGameMode::HandleMatchTravelReadyTimeout,
		MatchTravelReadyTimeoutSeconds,
		false);
}

bool AHeistLobbyGameMode::IsHostController(APlayerController* PlayerController) const
{
	if (!IsValid(GameState)) return false;
	if (GameState->PlayerArray.IsEmpty()) return false;

	const APlayerState* HostPlayerState = GameState->PlayerArray[0];
	if (!IsValid(HostPlayerState)) return false;

	return HostPlayerState->GetOwner() == PlayerController;
}

void AHeistLobbyGameMode::StartMatchTravel()
{
	if (!bStartGameRequested)
	{
		return;
	}

	ensureAlwaysMsgf(!GameMapPath.IsEmpty(), TEXT("AHeistLobbyGameMode: GameMapPath is not set."));

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		bStartGameRequested = false;
		return;
	}

	const FString TravelPath = FString::Printf(
		TEXT("%s?RequiredPlayersToStartBriefing=%d"),
		*GameMapPath,
		MinPlayersToStart);

	bStartGameRequested = false;
	World->GetTimerManager().ClearTimer(MatchTravelReadyTimeoutHandle);
	ExpectedPlayersForMatchTravel.Reset();
	PlayersReadyForMatchTravel.Reset();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UHeistVoiceSubsystem* VS = GI->GetSubsystem<UHeistVoiceSubsystem>())
		{
			VS->BeginTravelShutdown(World);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[HeistLobbyGameMode] Starting game. Traveling to: %s"), *TravelPath);
	World->ServerTravel(TravelPath);
}

void AHeistLobbyGameMode::NotifyPlayerReadyForMatchTravel(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController) || !bStartGameRequested)
	{
		return;
	}

	AHeistPlayerState* HeistPlayerState = PlayerController->GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPlayerState))
	{
		return;
	}

	if (!ExpectedPlayersForMatchTravel.Contains(HeistPlayerState))
	{
		UE_LOG(LogTemp, Warning, TEXT("[HeistLobbyGameMode] NotifyPlayerReadyForMatchTravel: unexpected PS=%s"),
			*GetNameSafe(HeistPlayerState));
		return;
	}

	PlayersReadyForMatchTravel.Add(HeistPlayerState);

	const int32 TotalPlayers = CountExpectedPlayersForMatchTravel();
	const int32 ReadyPlayers = CountReadyPlayersForMatchTravel();

	UE_LOG(LogTemp, Log, TEXT("[HeistLobbyGameMode] NotifyPlayerReadyForMatchTravel: PC=%s Ready=%d/%d"),
		*GetNameSafe(PlayerController),
		ReadyPlayers,
		TotalPlayers);

	if (TotalPlayers > 0 && ReadyPlayers >= TotalPlayers)
	{
		StartMatchTravel();
	}
}

void AHeistLobbyGameMode::HandleMatchTravelReadyTimeout()
{
	if (!bStartGameRequested)
	{
		return;
	}

	const int32 TotalPlayers = CountExpectedPlayersForMatchTravel();
	const int32 ReadyPlayers = CountReadyPlayersForMatchTravel();

	UE_LOG(LogTemp, Warning, TEXT("[HeistLobbyGameMode] Match travel ready timeout: Ready=%d/%d"),
		ReadyPlayers,
		TotalPlayers);

	for (const TWeakObjectPtr<APlayerState>& ExpectedPlayer : ExpectedPlayersForMatchTravel)
	{
		if (!ExpectedPlayer.IsValid() || PlayersReadyForMatchTravel.Contains(ExpectedPlayer))
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("[HeistLobbyGameMode] Match travel ready timeout: missing PS=%s"),
			*GetNameSafe(ExpectedPlayer.Get()));
	}

	bStartGameRequested = false;
	ExpectedPlayersForMatchTravel.Reset();
	PlayersReadyForMatchTravel.Reset();
}

int32 AHeistLobbyGameMode::CountExpectedPlayersForMatchTravel() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<APlayerState>& ExpectedPlayer : ExpectedPlayersForMatchTravel)
	{
		if (ExpectedPlayer.IsValid())
		{
			++Count;
		}
	}

	return Count;
}

int32 AHeistLobbyGameMode::CountReadyPlayersForMatchTravel() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<APlayerState>& ReadyPlayer : PlayersReadyForMatchTravel)
	{
		if (ReadyPlayer.IsValid())
		{
			++Count;
		}
	}

	return Count;
}
