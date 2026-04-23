#include "Core/HeistMatchGameMode.h"

#include "Components/HeistArrestVictoryComponent.h"
#include "Components/HeistBriefingPhaseComponent.h"
#include "Components/HeistExecutionPhaseComponent.h"
#include "Components/HeistPhaseManagerComponent.h"
#include "Components/HeistGameOverPhaseComponent.h"
#include "Components/HeistDropZoneManagerComponent.h"
#include "Core/HeistMatchGameState.h"
#include "Core/HeistPlayerController.h"
#include "Core/HeistPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AHeistMatchGameMode::AHeistMatchGameMode()
{
	bStartPlayersAsSpectators = true;
	bUseSeamlessTravel = true;

	PhaseManagerComponent = CreateDefaultSubobject<UHeistPhaseManagerComponent>(TEXT("PhaseManagerComponent"));
	BriefingPhaseComponent = CreateDefaultSubobject<UHeistBriefingPhaseComponent>(TEXT("BriefingPhaseComponent"));
	ExecutionPhaseComponent = CreateDefaultSubobject<UHeistExecutionPhaseComponent>(TEXT("ExecutionPhaseComponent"));
	ArrestVictoryComponent = CreateDefaultSubobject<UHeistArrestVictoryComponent>(TEXT("ArrestVictoryComponent"));
	GameOverPhaseComponent = CreateDefaultSubobject<UHeistGameOverPhaseComponent>(TEXT("GameOverPhaseComponent"));
	DropZoneManagerComponent = CreateDefaultSubobject<UHeistDropZoneManagerComponent>(TEXT("DropZoneManagerComponent"));
	LobbyMapPath = TEXT("/Game/Heist/Maps/L_Lobby");
}

void AHeistMatchGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FString RequiredPlayersOption = UGameplayStatics::ParseOption(Options, TEXT("RequiredPlayersToStartBriefing"));
	PendingRequiredPlayersToStartBriefing = RequiredPlayersOption.IsEmpty()
		? DefaultRequiredPlayersToStartBriefing
		: FMath::Max(FCString::Atoi(*RequiredPlayersOption), 1);
	PlayersReadyForBriefingStart.Reset();
	ExpectedPlayersForMatchTravel.Reset();
	PlayersReadyForMatchTravel.Reset();
	bMatchVictoryDeclared = false;
	bLobbyTravelRequested = false;

	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] InitGame: RequiredPlayersToStartBriefing=%d"), PendingRequiredPlayersToStartBriefing);
}

void AHeistMatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(ArrestVictoryComponent))
	{
		ArrestVictoryComponent->OnPoliceVictory.AddLambda([this]()
		{
			NotifyPoliceVictory(EHeistVictoryReason::PoliceArrest);
		});
	}

	// 브리핑 시작 위치는 서버가 먼저 확정해 둔다.
	// 실제 브리핑 진입은 TryStartBriefingFlow -> PhaseManager -> BriefingPhase 순으로 이어지고,
	// 클라는 그 결과를 replicated state/OnRep 훅으로 뒤늦게 수렴한다.
	GatherBriefingStartPoints();
	TryStartBriefingFlow();
	TryInitDropZone();
}

void AHeistMatchGameMode::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);

	TryStartBriefingFlow();
}

void AHeistMatchGameMode::NotifyPlayerReadyForBriefingStart(APlayerController* PlayerController)
{
	if (!HasAuthority() || !IsValid(PlayerController) || bBriefingFlowStarted)
	{
		return;
	}

	const AHeistMatchGameState* MatchGameState = GetGameState<AHeistMatchGameState>();
	if (!IsValid(MatchGameState) || MatchGameState->IsBriefingPhase() || MatchGameState->IsExecutionPhase())
	{
		return;
	}

	const AHeistPlayerState* HeistPS = PlayerController->GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS))
	{
		return;
	}

	PlayersReadyForBriefingStart.Add(PlayerController);

	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] NotifyPlayerReadyForBriefingStart: PC=%s Ready=%d/%d"),
		*GetNameSafe(PlayerController),
		CountPlayersReadyForBriefingStart(),
		PendingRequiredPlayersToStartBriefing);

	TryStartBriefingFlow();
}

void AHeistMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!IsValid(NewPlayer))
	{
		return;
	}

	const AHeistMatchGameState* MatchGameState = GetGameState<AHeistMatchGameState>();
	if (!IsValid(MatchGameState) || !MatchGameState->IsBriefingPhase())
	{
		return;
	}

	const AHeistPlayerState* HeistPS = NewPlayer->GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS))
	{
		return;
	}

	SpawnPlayerAtBriefingStart(NewPlayer, HeistPS->GetAssignedTeam());
}

void AHeistMatchGameMode::Logout(AController* Exiting)
{
	const UWorld* World = GetWorld();

	AHeistPlayerState* HeistPS = Exiting ? Exiting->GetPlayerState<AHeistPlayerState>() : nullptr;
	const AHeistMatchGameState* MatchGameState = GetGameState<AHeistMatchGameState>();
	const bool bShouldNotifyDisconnect =
		IsValid(World)
		&& !World->bIsTearingDown
		&& IsValid(ArrestVictoryComponent)
		&& IsValid(HeistPS)
		&& HeistPS->IsThief()
		&& IsValid(MatchGameState)
		&& MatchGameState->IsExecutionPhase();

	if (bShouldNotifyDisconnect)
	{
		ArrestVictoryComponent->NotifyThiefDisconnected(HeistPS);
	}

	Super::Logout(Exiting);
}

UClass* AHeistMatchGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (!IsValid(InController))
	{
		return Super::GetDefaultPawnClassForController_Implementation(InController);
	}

	const AHeistPlayerState* HeistPS = InController->GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS) || HeistPS->GetAssignedTeam() == EHeistTeam::None)
	{
		return Super::GetDefaultPawnClassForController_Implementation(InController);
	}

	if (HeistPS->IsPolice() && IsValid(PolicePawnClass))
	{
		return PolicePawnClass;
	}

	if (HeistPS->IsThief() && IsValid(ThiefPawnClass))
	{
		return ThiefPawnClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AHeistMatchGameMode::TryStartBriefingFlow()
{
	if (bBriefingFlowStarted || !HasAuthority())
	{
		return;
	}

	if (!IsValid(PhaseManagerComponent) || CountPlayersReadyForBriefingStart() < PendingRequiredPlayersToStartBriefing)
	{
		return;
	}

	bBriefingFlowStarted = true;
	GatherBriefingStartPoints();
	// MatchGameMode는 브리핑 "진입"만 연다.
	// 실제 서버 시퀀스는 BriefingPhase 안에서
	// 팀 배정 -> 폰 스폰/재시작 -> 브리핑 컨텍스트 바인딩
	// 순으로 진행되며, 클라는 그 결과를 OnRep/possession 훅으로 따라잡는다.
	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] TryStartBriefingFlow: phase flow started"));
	PhaseManagerComponent->StartMatchFlow();
}

void AHeistMatchGameMode::NotifyPlayerReadyForMatchTravel(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController) || !bLobbyTravelRequested) return;

	AHeistPlayerState* HeistPlayerState = PlayerController->GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPlayerState)) return;

	if (!ExpectedPlayersForMatchTravel.Contains(HeistPlayerState))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchGameMode] NotifyPlayerReadyForMatchTravel: unexpected PS=%s"),
			*GetNameSafe(HeistPlayerState));
		return;
	}

	PlayersReadyForMatchTravel.Add(HeistPlayerState);

	const int32 TotalPlayers = CountExpectedPlayersForMatchTravel();
	const int32 ReadyPlayers = CountReadyPlayersForMatchTravel();

	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] NotifyPlayerReadyForMatchTravel: PC=%s Ready=%d/%d"),
		*GetNameSafe(PlayerController),
		ReadyPlayers,
		TotalPlayers);

	if (TotalPlayers > 0 && ReadyPlayers >= TotalPlayers)
	{
		StartLobbyTravel();
	}
}

void AHeistMatchGameMode::NotifyPoliceVictory(EHeistVictoryReason Reason)
{
	NotifyVictory(EHeistTeam::Police, Reason);
}

void AHeistMatchGameMode::NotifyThiefVictory(EHeistVictoryReason Reason)
{
	NotifyVictory(EHeistTeam::Thief, Reason);
}

int32 AHeistMatchGameMode::CountPlayersReadyForBriefingStart() const
{
	int32 ReadyPlayerCount = 0;

	for (const TWeakObjectPtr<APlayerController>& PlayerController : PlayersReadyForBriefingStart)
	{
		if (PlayerController.IsValid())
		{
			++ReadyPlayerCount;
		}
	}

	return ReadyPlayerCount;
}

int32 AHeistMatchGameMode::CountExpectedPlayersForMatchTravel() const
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

int32 AHeistMatchGameMode::CountReadyPlayersForMatchTravel() const
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

int32 AHeistMatchGameMode::CountSettledMatchPlayers() const
{
	if (!IsValid(GameState))
	{
		return 0;
	}

	int32 SettledPlayerCount = 0;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPS))
		{
			continue;
		}

		if (!IsValid(Cast<APlayerController>(HeistPS->GetOwner())))
		{
			continue;
		}

		++SettledPlayerCount;
	}

	return SettledPlayerCount;
}

void AHeistMatchGameMode::GatherBriefingStartPoints()
{
	ThiefBriefingStartPoint = nullptr;
	PoliceBriefingStartPoint = nullptr;

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(World, ThiefBriefingStartTag, FoundActors);
	if (!FoundActors.IsEmpty())
	{
		ThiefBriefingStartPoint = FoundActors[0];
	}

	FoundActors.Reset();
	UGameplayStatics::GetAllActorsWithTag(World, PoliceBriefingStartTag, FoundActors);
	if (!FoundActors.IsEmpty())
	{
		PoliceBriefingStartPoint = FoundActors[0];
	}
}

AActor* AHeistMatchGameMode::FindBriefingStartPoint(EHeistTeam Team) const
{
	switch (Team)
	{
	case EHeistTeam::Thief:
		return ThiefBriefingStartPoint;

	case EHeistTeam::Police:
		return PoliceBriefingStartPoint;

	default:
		return nullptr;
	}
}

void AHeistMatchGameMode::SpawnPlayerAtBriefingStart(APlayerController* PlayerController, EHeistTeam Team)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	if (Team != EHeistTeam::Thief && Team != EHeistTeam::Police)
	{
		return;
	}

	AActor* StartPoint = FindBriefingStartPoint(Team);
	if (!IsValid(StartPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("HeistMatchGameMode: 브리핑 시작 지점을 찾지 못했습니다. Team=%d"), static_cast<int32>(Team));
		return;
	}

	if (APawn* ExistingPawn = PlayerController->GetPawn())
	{
		ExistingPawn->Destroy();
	}

	// 브리핑 진입 시점의 Pawn 생성/재시작은 서버가 authoritative 하게 수행한다.
	// 이후 클라는 PlayerState/Controller/Pawn 복제를 순서 없이 받으므로,
	// AcknowledgePossession / OnRep_PlayerState / OnRep_Controller 같은 훅에서 최종 상태로 수렴한다.
	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] SpawnPlayerAtBriefingStart: PC=%s Team=%d Start=%s"),
		*PlayerController->GetName(),
		static_cast<int32>(Team),
		*StartPoint->GetName());
	RestartPlayerAtTransform(PlayerController, StartPoint->GetActorTransform());
}

void AHeistMatchGameMode::SpawnAllPlayersAtBriefingStart()
{
	const AHeistMatchGameState* MatchGameState = GetGameState<AHeistMatchGameState>();
	if (!IsValid(MatchGameState) || !MatchGameState->IsBriefingPhase())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchGameMode] SpawnAllPlayersAtBriefingStart: skipped, briefing phase not ready"));
		return;
	}

	// 이 호출은 BriefingPhase의 팀 배정 직후 실행된다.
	// 즉 서버 기준 브리핑 시작 순서는
	// 팀 배정 -> 브리핑용 Pawn 스폰/재시작 -> 브리핑 액터/컨텍스트 바인딩
	// 이며, 이후 UI는 클라 훅 쪽에서 readiness를 재평가하며 열린다.
	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] SpawnAllPlayersAtBriefingStart: begin"));
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPS))
		{
			continue;
		}

		APlayerController* PlayerController = Cast<APlayerController>(HeistPS->GetOwner());
		if (!IsValid(PlayerController))
		{
			continue;
		}

		SpawnPlayerAtBriefingStart(PlayerController, HeistPS->GetAssignedTeam());
	}
	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] SpawnAllPlayersAtBriefingStart: end"));
}

void AHeistMatchGameMode::TryEngineChannelingStart()
{
	if (!IsValid(GameOverPhaseComponent)) return;
	GameOverPhaseComponent->StartEngineChanneling();

	AHeistMatchGameState* MatchGameState = GetGameState<AHeistMatchGameState>();
	MatchGameState->SetEngineChannelingStart(true);
}

void AHeistMatchGameMode::RequestVehicleEscapeSequence(int32 GroupIndex)
{
	if (!HasAuthority() || bMatchVictoryDeclared || bVehicleEscapeSequenceRequested) return;
	if (GroupIndex == INDEX_NONE) return;

	if (!OnVehicleEscapeSequenceRequested.IsBound())
	{
		UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] Vehicle escape sequence is not bound. Falling back to immediate score judgment. GroupIndex=%d"), GroupIndex);
		JudgeScore(GroupIndex);
		return;
	}

	bVehicleEscapeSequenceRequested = true;
	PendingVehicleEscapeGroupIndex = GroupIndex;

	if (IsValid(PhaseManagerComponent))
	{
		PhaseManagerComponent->StopActiveTimers();
	}

	SetAllPlayersCinematicMode(true);
	BroadcastVehicleEscapeSequenceToPlayers(GroupIndex);
	OnVehicleEscapeSequenceRequested.Broadcast(GroupIndex);

	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] Vehicle escape sequence requested. GroupIndex=%d"), GroupIndex);
}

void AHeistMatchGameMode::JudgeScore(int32 GroupIndex)
{
	if (!HasAuthority() || bMatchVictoryDeclared) return;
	if (!IsValid(DropZoneManagerComponent)) return;

	if (bVehicleEscapeSequenceRequested && PendingVehicleEscapeGroupIndex != INDEX_NONE && PendingVehicleEscapeGroupIndex != GroupIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchGameMode] JudgeScore called with mismatched GroupIndex. Requested=%d, Actual=%d"),
			PendingVehicleEscapeGroupIndex,
			GroupIndex);
	}

	bVehicleEscapeSequenceRequested = false;
	PendingVehicleEscapeGroupIndex = INDEX_NONE;

	int32 ZoneIndex = DropZoneManagerComponent->GetZoneIndexByGroup(GroupIndex);

	AHeistMatchGameState* HeistGS = GetGameState<AHeistMatchGameState>();
	if (HeistGS->GetZoneScore(ZoneIndex).CurrentScore >= HeistGS->GetZoneScore(ZoneIndex).TargetScore && !DropZoneManagerComponent->CheckDoorOpened(GroupIndex))
	{
		NotifyThiefVictory(EHeistVictoryReason::ThiefEscape);
	}
	else
	{
		NotifyPoliceVictory(EHeistVictoryReason::PoliceScoreWin);
	}
}

void AHeistMatchGameMode::NotifyVictory(EHeistTeam Winner, EHeistVictoryReason Reason)
{
	if (!HasAuthority() || bMatchVictoryDeclared) return;

	bMatchVictoryDeclared = true;
	bVehicleEscapeSequenceRequested = false;
	PendingVehicleEscapeGroupIndex = INDEX_NONE;

	if (IsValid(PhaseManagerComponent))
	{
		PhaseManagerComponent->StopActiveTimers();
	}

	if (AHeistMatchGameState* MatchGameState = GetGameState<AHeistMatchGameState>())
	{
		MatchGameState->SetCurrentPhase(EHeistMatchPhase::Result);
		MatchGameState->SetBriefingSelectionLocked(true);
		MatchGameState->SetPhaseRemainingTime(0.f);
		MatchGameState->SetPhaseEndServerTime(0.f);
	}

	SetAllPlayersCinematicMode(true);
	BroadcastMatchResultToPlayers(Winner, Reason);

	OnMatchVictory.Broadcast(Winner);
	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] Match victory declared. Winner=%d Reason=%d"),
		static_cast<int32>(Winner),
		static_cast<int32>(Reason));

	if (ResultScreenDuration <= 0.f)
	{
		StartReturnToLobbyFlow();
		return;
	}

	GetWorldTimerManager().ClearTimer(ResultScreenTimerHandle);
	GetWorldTimerManager().SetTimer(
		ResultScreenTimerHandle,
		this,
		&ThisClass::StartReturnToLobbyFlow,
		ResultScreenDuration,
		false);
}

void AHeistMatchGameMode::SetAllPlayersCinematicMode(bool bEnable)
{
	if (!IsValid(GameState)) return;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AHeistPlayerController* HeistPC = Cast<AHeistPlayerController>(PlayerState ? PlayerState->GetOwner() : nullptr);
		if (!IsValid(HeistPC)) continue;

		HeistPC->SetCinematicMode(bEnable, false, false, true, true);
	}
}

void AHeistMatchGameMode::BroadcastVehicleEscapeSequenceToPlayers(int32 GroupIndex)
{
	if (!IsValid(GameState)) return;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AHeistPlayerController* HeistPC = Cast<AHeistPlayerController>(PlayerState ? PlayerState->GetOwner() : nullptr);
		if (!IsValid(HeistPC)) continue;

		HeistPC->ClientBeginVehicleEscapeCinematic(GroupIndex);
	}
}

void AHeistMatchGameMode::BroadcastMatchResultToPlayers(EHeistTeam Winner, EHeistVictoryReason Reason)
{
	if (!IsValid(GameState)) return;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AHeistPlayerController* HeistPC = Cast<AHeistPlayerController>(PlayerState ? PlayerState->GetOwner() : nullptr);
		if (!IsValid(HeistPC)) continue;

		HeistPC->ClientNotifyMatchResult(Winner, Reason);
	}
}

bool AHeistMatchGameMode::TryCheckDoorMoving(int32 GroupIndex)
{
	if (!IsValid(DropZoneManagerComponent)) return false;
	return DropZoneManagerComponent->CheckDoorMoving(GroupIndex);
}

bool AHeistMatchGameMode::TryCheckDoorOpened(int32 GroupIndex)
{
	if (!IsValid(DropZoneManagerComponent)) return false;
	return DropZoneManagerComponent->CheckDoorOpened(GroupIndex);
}

void AHeistMatchGameMode::TryCloseDoor(int32 GroupIndex)
{
	if (!IsValid(DropZoneManagerComponent)) return;
	DropZoneManagerComponent->CloseDoor(GroupIndex);
}

void AHeistMatchGameMode::TryOpenDoor(int32 GroupIndex)
{
	if (!IsValid(DropZoneManagerComponent)) return;
	DropZoneManagerComponent->OpenDoor(GroupIndex);
}

void AHeistMatchGameMode::StartReturnToLobbyFlow()
{
	if (!HasAuthority() || bLobbyTravelRequested) return;

	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(GameState)) return;

	bLobbyTravelRequested = true;
	ExpectedPlayersForMatchTravel.Reset();
	PlayersReadyForMatchTravel.Reset();

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AHeistPlayerState* HeistPlayerState = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPlayerState)) continue;

		AHeistPlayerController* HeistPC = Cast<AHeistPlayerController>(HeistPlayerState->GetOwner());
		if (!IsValid(HeistPC)) continue;

		ExpectedPlayersForMatchTravel.Add(HeistPlayerState);
		HeistPC->ClientPrepareForMatchTravel();
	}

	if (ExpectedPlayersForMatchTravel.IsEmpty())
	{
		StartLobbyTravel();
		return;
	}

	World->GetTimerManager().SetTimer(
		LobbyTravelReadyTimeoutHandle,
		this,
		&ThisClass::HandleLobbyTravelReadyTimeout,
		LobbyTravelReadyTimeoutSeconds,
		false);
}

void AHeistMatchGameMode::StartLobbyTravel()
{
	if (!bLobbyTravelRequested) return;

	UWorld* World = GetWorld();
	if (!IsValid(World) || LobbyMapPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchGameMode] StartLobbyTravel: LobbyMapPath is not available."));
		bLobbyTravelRequested = false;
		return;
	}

	const bool bHasQueryString = LobbyMapPath.Contains(TEXT("?"));
	const FString TravelPath = LobbyMapPath + (bHasQueryString ? TEXT("&listen") : TEXT("?listen"));

	bLobbyTravelRequested = false;
	World->GetTimerManager().ClearTimer(LobbyTravelReadyTimeoutHandle);
	ExpectedPlayersForMatchTravel.Reset();
	PlayersReadyForMatchTravel.Reset();

	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] Returning to lobby. Traveling to: %s"), *TravelPath);
	World->ServerTravel(TravelPath);
}

void AHeistMatchGameMode::HandleLobbyTravelReadyTimeout()
{
	if (!bLobbyTravelRequested) return;

	UE_LOG(LogTemp, Warning, TEXT("[MatchGameMode] Lobby travel ready timeout: Ready=%d/%d"),
		CountReadyPlayersForMatchTravel(),
		CountExpectedPlayersForMatchTravel());

	for (const TWeakObjectPtr<APlayerState>& ExpectedPlayer : ExpectedPlayersForMatchTravel)
	{
		if (!ExpectedPlayer.IsValid() || PlayersReadyForMatchTravel.Contains(ExpectedPlayer)) continue;

		UE_LOG(LogTemp, Warning, TEXT("[MatchGameMode] Lobby travel ready timeout: missing PS=%s"),
			*GetNameSafe(ExpectedPlayer.Get()));
	}

	StartLobbyTravel();
}

void AHeistMatchGameMode::TryInitDropZone()
{
	if (!IsValid(DropZoneManagerComponent)) return;
	DropZoneManagerComponent->InitDropZone();
}
