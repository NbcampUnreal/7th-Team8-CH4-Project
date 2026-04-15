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

void AHeistMatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	GatherBriefingStartPoints();
	TryStartBriefingFlow();
}

void AHeistMatchGameMode::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);

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

void AHeistMatchGameMode::TryStartBriefingFlow()
{
	if (bBriefingFlowStarted || !HasAuthority())
	{
		return;
	}

	if (!IsValid(PhaseManagerComponent) || CountSettledMatchPlayers() < RequiredPlayersToStartBriefing)
	{
		return;
	}

	bBriefingFlowStarted = true;
	PhaseManagerComponent->StartMatchFlow();
	SpawnAllPlayersAtBriefingStart();
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

	RestartPlayerAtTransform(PlayerController, StartPoint->GetActorTransform());
}

void AHeistMatchGameMode::SpawnAllPlayersAtBriefingStart()
{
	const AHeistMatchGameState* MatchGameState = GetGameState<AHeistMatchGameState>();
	if (!IsValid(MatchGameState) || !MatchGameState->IsBriefingPhase())
	{
		return;
	}

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
}
