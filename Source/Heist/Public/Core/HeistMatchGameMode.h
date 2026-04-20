
#pragma once

#include "CoreMinimal.h"
#include "Core/HeistMatchTypes.h"
#include "GameFramework/GameModeBase.h"
#include "HeistMatchGameMode.generated.h"

class UHeistArrestVictoryComponent;
class UHeistPhaseManagerComponent;
class UHeistBriefingPhaseComponent;
class UHeistExecutionPhaseComponent;
class APlayerStart;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMatchVictory, EHeistTeam /*Winner*/);

/**
 *
 */
UCLASS()
class HEIST_API AHeistMatchGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHeistMatchGameMode();

	FOnMatchVictory OnMatchVictory;

	void NotifyPlayerReadyForBriefingStart(APlayerController* PlayerController);
	void NotifyPlayerReadyForMatchTravel(APlayerController* PlayerController);
	void NotifyPoliceVictory();
	void SpawnAllPlayersAtBriefingStart();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void GenericPlayerInitialization(AController* C) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Heist|Components")
	TObjectPtr<UHeistPhaseManagerComponent> PhaseManagerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Heist|Components")
	TObjectPtr<UHeistBriefingPhaseComponent> BriefingPhaseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Heist|Components")
	TObjectPtr<UHeistExecutionPhaseComponent> ExecutionPhaseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Heist|Components")
	TObjectPtr<UHeistArrestVictoryComponent> ArrestVictoryComponent;

	void GatherBriefingStartPoints();
	void TryStartBriefingFlow();
	int32 CountPlayersReadyForBriefingStart() const;
	int32 CountSettledMatchPlayers() const;
	AActor* FindBriefingStartPoint(EHeistTeam Team) const;
	void SpawnPlayerAtBriefingStart(APlayerController* PlayerController, EHeistTeam Team);
	void StartReturnToLobbyFlow();
	void StartLobbyTravel();
	void HandleLobbyTravelReadyTimeout();
	int32 CountExpectedPlayersForMatchTravel() const;
	int32 CountReadyPlayersForMatchTravel() const;

	UPROPERTY(EditDefaultsOnly, Category="Heist|Briefing")
	FName ThiefBriefingStartTag = TEXT("StartPoint_Briefing_Thief");

	UPROPERTY(EditDefaultsOnly, Category="Heist|Briefing")
	FName PoliceBriefingStartTag = TEXT("StartPoint_Briefing_Police");

	UPROPERTY(EditDefaultsOnly, Category="Heist|Briefing", meta=(ClampMin="1"))
	int32 DefaultRequiredPlayersToStartBriefing = 1;

	int32 PendingRequiredPlayersToStartBriefing = 1;

	UPROPERTY(EditDefaultsOnly, Category="Heist|Pawn")
	TSubclassOf<APawn> ThiefPawnClass;

	UPROPERTY(EditDefaultsOnly, Category="Heist|Pawn")
	TSubclassOf<APawn> PolicePawnClass;

	UPROPERTY()
	TObjectPtr<AActor> ThiefBriefingStartPoint;

	UPROPERTY()
	TObjectPtr<AActor> PoliceBriefingStartPoint;

	bool bBriefingFlowStarted = false;
	bool bMatchVictoryDeclared = false;
	bool bLobbyTravelRequested = false;
	TSet<TWeakObjectPtr<APlayerController>> PlayersReadyForBriefingStart;
	TSet<TWeakObjectPtr<APlayerState>> ExpectedPlayersForMatchTravel;
	TSet<TWeakObjectPtr<APlayerState>> PlayersReadyForMatchTravel;

	UPROPERTY(EditDefaultsOnly, Category="Heist|Travel")
	float LobbyTravelReadyTimeoutSeconds = 5.0f;

	FTimerHandle LobbyTravelReadyTimeoutHandle;
};
