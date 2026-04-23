#include "Core/HeistLobbyGameMode.h"

#include "Core/HeistLobbyGameState.h"
#include "Core/HeistPlayerController.h"
#include "Core/HeistPlayerState.h"
#include "Components/FlashlightComponent.h"
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

void AHeistLobbyGameMode::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);

	APlayerController* PC = Cast<APlayerController>(C);
	if (!IsValid(PC)) return;

	AHeistPlayerState* PlayerState = PC->GetPlayerState<AHeistPlayerState>();
	if (!IsValid(PlayerState)) return;

	if (IsHostController(PC))
	{
		PlayerState->SetIsHost(true);
		PlayerState->SetIsReady(true);
	}
}

void AHeistLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!IsValid(NewPlayer)) return;

	const AHeistPlayerState* PlayerState = NewPlayer->GetPlayerState<AHeistPlayerState>();
	const FString PlayerName = IsValid(PlayerState) ? PlayerState->GetPlayerName() : TEXT("Unknown");

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

void AHeistLobbyGameMode::RequestTogglePreviewCharacter(APlayerController* Requester)
{
	if (!IsValid(Requester)) return;
	if (bStartGameRequested) return;

	APawn* CurrentPawn = Requester->GetPawn();
	if (!IsValid(CurrentPawn)) return;

	TSubclassOf<APawn> TargetClass = nullptr;
	if (IsValid(LobbyThiefCharacterClass) && CurrentPawn->IsA(LobbyThiefCharacterClass))
	{
		TargetClass = LobbyPoliceCharacterClass;
	}
	else if (IsValid(LobbyPoliceCharacterClass) && CurrentPawn->IsA(LobbyPoliceCharacterClass))
	{
		TargetClass = LobbyThiefCharacterClass;
	}

	if (!IsValid(TargetClass)) return;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	const FVector SpawnLocation = CurrentPawn->GetActorLocation();
	const FRotator SpawnRotation = CurrentPawn->GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* NewPawn = World->SpawnActor<APawn>(TargetClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!IsValid(NewPawn)) return;

	APawn* OldPawn = Requester->GetPawn();
	Requester->Possess(NewPawn);
	if (IsValid(OldPawn))
	{
		if (UFlashlightComponent* Flashlight = OldPawn->GetComponentByClass<UFlashlightComponent>())
		{
			Flashlight->StopLocalVision();
			Flashlight->RestoreAllThiefVisibility();
		}
		OldPawn->Destroy();
	}
}

bool AHeistLobbyGameMode::IsHostController(APlayerController* PlayerController) const
{
	// Listen Server에서 방장은 로컬 플레이어이다.
	// PlayerArray[0] 기반 판별은 시임리스 트래블 귀환 시 원격 클라이언트가
	// 먼저 초기화되어 인덱스 0을 점유할 수 있으므로 신뢰할 수 없다.
	return IsValid(PlayerController) && PlayerController->IsLocalController();
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
