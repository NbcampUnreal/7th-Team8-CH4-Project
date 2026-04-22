
#pragma once

#include "CoreMinimal.h"
#include "Core/HeistMatchTypes.h"
#include "GameFramework/GameModeBase.h"
#include "HeistMatchGameMode.generated.h"

class UHeistArrestVictoryComponent;
class UHeistPhaseManagerComponent;
class UHeistBriefingPhaseComponent;
class UHeistExecutionPhaseComponent;
class UHeistGameOverPhaseComponent;
class UHeistDropZoneManagerComponent;
class APlayerStart;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMatchVictory, EHeistTeam /*Winner*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehicleEscapeSequenceRequested, int32, GroupIndex);

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

	UPROPERTY(BlueprintAssignable, Category = "Heist|GameEnd")
	FOnVehicleEscapeSequenceRequested OnVehicleEscapeSequenceRequested;

	void NotifyPlayerReadyForBriefingStart(APlayerController* PlayerController);
	void NotifyPlayerReadyForMatchTravel(APlayerController* PlayerController);
	void NotifyPoliceVictory(EHeistVictoryReason Reason = EHeistVictoryReason::PoliceArrest);
	void NotifyThiefVictory(EHeistVictoryReason Reason = EHeistVictoryReason::ThiefEscape);
	void SpawnAllPlayersAtBriefingStart();
	void TryEngineChannelingStart();

	UFUNCTION(BlueprintCallable, Category = "Heist|GameEnd")
	void RequestVehicleEscapeSequence(int32 GroupIndex);

	// Vehicle 컷신 종료 후 레벨 BP가 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Heist|GameEnd")
	void JudgeScore(int32 GroupIndex);
	bool TryCheckDoorMoving(int32 GroupIndex);
	bool TryCheckDoorOpened(int32 GroupIndex);
	void TryCloseDoor(int32 GroupIndex);
	void TryOpenDoor(int32 GroupIndex);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components")
	TObjectPtr<UHeistGameOverPhaseComponent> GameOverPhaseComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Heist|Components")
	TObjectPtr<UHeistArrestVictoryComponent> ArrestVictoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components")
	TObjectPtr<UHeistDropZoneManagerComponent> DropZoneManagerComponent;

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
	void NotifyVictory(EHeistTeam Winner, EHeistVictoryReason Reason);
	void SetAllPlayersCinematicMode(bool bEnable);
	void BroadcastVehicleEscapeSequenceToPlayers(int32 GroupIndex);
	void BroadcastMatchResultToPlayers(EHeistTeam Winner, EHeistVictoryReason Reason);

	void TryInitDropZone();

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

	UPROPERTY(EditDefaultsOnly, Category = "Heist|GameEnd", meta = (ClampMin = "0.0"))
	float ResultScreenDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category="Heist|Travel")
	FString LobbyMapPath;

	FTimerHandle LobbyTravelReadyTimeoutHandle;
	FTimerHandle ResultScreenTimerHandle;
	int32 PendingVehicleEscapeGroupIndex = INDEX_NONE;
	bool bVehicleEscapeSequenceRequested = false;
};
