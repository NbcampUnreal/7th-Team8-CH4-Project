
#pragma once

#include "CoreMinimal.h"
#include "Core/HeistMatchTypes.h"
#include "GameFramework/GameModeBase.h"
#include "HeistMatchGameMode.generated.h"

class UHeistPhaseManagerComponent;
class UHeistBriefingPhaseComponent;
class UHeistExecutionPhaseComponent;
class APlayerStart;

/**
 *
 */
UCLASS()
class HEIST_API AHeistMatchGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHeistMatchGameMode();

	void NotifyPlayerReadyForBriefingStart(APlayerController* PlayerController);
	void SpawnAllPlayersAtBriefingStart();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void GenericPlayerInitialization(AController* C) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Heist|Components")
	TObjectPtr<UHeistPhaseManagerComponent> PhaseManagerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Heist|Components")
	TObjectPtr<UHeistBriefingPhaseComponent> BriefingPhaseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Heist|Components")
	TObjectPtr<UHeistExecutionPhaseComponent> ExecutionPhaseComponent;

	void GatherBriefingStartPoints();
	void TryStartBriefingFlow();
	int32 CountPlayersReadyForBriefingStart() const;
	int32 CountSettledMatchPlayers() const;
	AActor* FindBriefingStartPoint(EHeistTeam Team) const;
	void SpawnPlayerAtBriefingStart(APlayerController* PlayerController, EHeistTeam Team);

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
	TSet<TWeakObjectPtr<APlayerController>> PlayersReadyForBriefingStart;
};
