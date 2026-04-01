#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ArenaGameMode.generated.h"

// Broadcasts remaining seconds every tick of the countdown (once per second)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnArenaCountdownTick, int32 /*RemainingSeconds*/);

// Broadcasts when the match transitions to InProgress (player count reached or timeout)
DECLARE_MULTICAST_DELEGATE(FOnArenaMatchStarting);

/**
 * GameMode for the waiting lobby (Arena).
 * Uses AGameMode's match state machine: stays in WaitingToStart until
 * ReadyToStartMatch() returns true, then transitions to InProgress automatically.
 *
 * Setup in Blueprint Class Defaults:
 *   - MinPlayersToStart: player count that triggers the match start
 *   - MaxPlayers (inherited, in GameMode category): hard cap on connections — should match NumPublicConnections in your session config
 *   - StartTimeoutSeconds: countdown duration shown in UI (0 = wait indefinitely)
 */
UCLASS()
class MULTIPLAYERSESSIONS_API AArenaGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AArenaGameMode();

	// Bind to these on the server to drive lobby UI
	FOnArenaCountdownTick OnArenaCountdownTick;
	FOnArenaMatchStarting OnArenaMatchStarting;

protected:
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;

private:
	// Minimum number of players required to start the game.
	UPROPERTY(EditDefaultsOnly, Category = "Arena", meta = (ClampMin = 1))
	int32 MinPlayersToStart = 5;

	// Seconds before the match starts regardless of player count. 0 = wait indefinitely.
	UPROPERTY(EditDefaultsOnly, Category = "Arena", meta = (ClampMin = 0))
	float StartTimeoutSeconds = 60.f;

	int32 CurrentPlayerCount = 0;
	FTimerHandle StartTimeoutHandle;
	FTimerHandle CountdownTickHandle;

	void OnStartTimeout();
	void OnCountdownTick();
};
