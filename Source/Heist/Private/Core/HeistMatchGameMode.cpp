#include "Core/HeistMatchGameMode.h"

#include "Components/HeistBriefingPhaseComponent.h"
#include "Components/HeistExecutionPhaseComponent.h"
#include "Components/HeistPhaseManagerComponent.h"
#include "Core/HeistMatchGameState.h"
#include "Core/HeistPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AHeistMatchGameMode::AHeistMatchGameMode()
{
	bStartPlayersAsSpectators = true;

	PhaseManagerComponent = CreateDefaultSubobject<UHeistPhaseManagerComponent>(TEXT("PhaseManagerComponent"));
	BriefingPhaseComponent = CreateDefaultSubobject<UHeistBriefingPhaseComponent>(TEXT("BriefingPhaseComponent"));
	ExecutionPhaseComponent = CreateDefaultSubobject<UHeistExecutionPhaseComponent>(TEXT("ExecutionPhaseComponent"));
}

void AHeistMatchGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FString RequiredPlayersOption = UGameplayStatics::ParseOption(Options, TEXT("RequiredPlayersToStartBriefing"));
	PendingRequiredPlayersToStartBriefing = RequiredPlayersOption.IsEmpty()
		? DefaultRequiredPlayersToStartBriefing
		: FMath::Max(FCString::Atoi(*RequiredPlayersOption), 1);
	PlayersReadyForBriefingStart.Reset();

	UE_LOG(LogTemp, Log, TEXT("[MatchGameMode] InitGame: RequiredPlayersToStartBriefing=%d"), PendingRequiredPlayersToStartBriefing);
}

void AHeistMatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 브리핑 시작 위치는 서버가 먼저 확정해 둔다.
	// 실제 브리핑 진입은 TryStartBriefingFlow -> PhaseManager -> BriefingPhase 순으로 이어지고,
	// 클라는 그 결과를 replicated state/OnRep 훅으로 뒤늦게 수렴한다.
	GatherBriefingStartPoints();
	TryStartBriefingFlow();
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
