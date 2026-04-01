#include "ArenaGameMode.h"
#include "MultiplayerSessionsSubsystem.h"

AArenaGameMode::AArenaGameMode()
{
	// Hold in WaitingToStart until ReadyToStartMatch() returns true.
	// The engine polls ReadyToStartMatch() every tick and calls StartMatch() automatically.
	bDelayedStart = true;
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (StartTimeoutSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(StartTimeoutHandle, this, &AArenaGameMode::OnStartTimeout, StartTimeoutSeconds, false);
		GetWorldTimerManager().SetTimer(CountdownTickHandle, this, &AArenaGameMode::OnCountdownTick, 1.f, true);
	}
}

void AArenaGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	++CurrentPlayerCount;
}

void AArenaGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	--CurrentPlayerCount;
}

bool AArenaGameMode::ReadyToStartMatch_Implementation()
{
	return CurrentPlayerCount >= MinPlayersToStart;
}

void AArenaGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	GetWorldTimerManager().ClearTimer(StartTimeoutHandle);
	GetWorldTimerManager().ClearTimer(CountdownTickHandle);

	OnArenaMatchStarting.Broadcast();

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UMultiplayerSessionsSubsystem* Subsystem = GI->GetSubsystem<UMultiplayerSessionsSubsystem>();
	if (!Subsystem) return;

	Subsystem->StartSession();
}

void AArenaGameMode::OnStartTimeout()
{
	UE_LOG(LogGameMode, Log, TEXT("AArenaGameMode: Start timeout reached. Starting match with %d/%d players."),
		CurrentPlayerCount, MinPlayersToStart);

	StartMatch();
}

void AArenaGameMode::OnCountdownTick()
{
	const int32 Remaining = FMath::CeilToInt(GetWorldTimerManager().GetTimerRemaining(StartTimeoutHandle));
	OnArenaCountdownTick.Broadcast(Remaining);
}
