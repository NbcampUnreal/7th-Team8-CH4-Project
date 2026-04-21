
#pragma once

#include "CoreMinimal.h"
#include "HeistMatchTypes.h"
#include "GameFramework/GameStateBase.h"
#include "HeistMatchGameState.generated.h"

/**
 * 
 */
UCLASS()
class HEIST_API AHeistMatchGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	void InitZoneScores(int32 ZoneVolumeCount, int32 TargetScore);

	void SetCurrentPhase(EHeistMatchPhase InPhase);
	void SetPhaseRemainingTime(float InRemainingTime);
	void SetBriefingSelectionLocked(bool bLocked);
	void SetPhaseEndServerTime(float InPhaseEndServerTime);
	void SetZoneScore(int32 ZoneIndex, int32 NewScore);
	void SetEngineChannelingStart(bool bStart);
	void SetEngineChannelingEnd(bool bEnd);

	UFUNCTION(BlueprintPure)
	bool IsBriefingPhase() const { return CurrentPhase == EHeistMatchPhase::Briefing; }
	
	UFUNCTION(BlueprintPure)
	bool IsExecutionPhase() const { return CurrentPhase == EHeistMatchPhase::Execution; }
	
	UFUNCTION(BlueprintPure)
	bool IsBriefingSelectionLocked() const { return bBriefingSelectionLocked; }
	
	UFUNCTION(BlueprintPure)
	float GetPhaseRemainingTime() const { return PhaseRemainingTime; }

	UFUNCTION(BlueprintPure)
	float GetPhaseEndServerTime() const { return PhaseEndServerTime; }

	UFUNCTION(BlueprintPure)
	FZoneScoreData GetZoneScore(int32 Index) const { return ZoneScores.IsValidIndex(Index) ? ZoneScores[Index] : FZoneScoreData(); }

	UFUNCTION(BlueprintPure)
	bool IsEngineChannelingStarted() const { return bEngineChannelingStarted; }

	UFUNCTION(BlueprintPure)
	bool IsEngineChannelingEnded() const { return bEngineChannelingEnded; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	void StartPhaseUiTimer();
	void StopPhaseUiTimer();
	void BroadcastPhaseTimeUpdated();

	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly)
	EHeistMatchPhase CurrentPhase = EHeistMatchPhase::None;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	float PhaseRemainingTime = 0.f;
	
	UPROPERTY(ReplicatedUsing = OnRep_BriefingSelectionLocked, BlueprintReadOnly)
	bool bBriefingSelectionLocked = false;

	UPROPERTY(ReplicatedUsing = OnRep_PhaseEndServerTime, BlueprintReadOnly)
	float PhaseEndServerTime = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_ZoneScores, BlueprintReadOnly)
	TArray<FZoneScoreData> ZoneScores;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bEngineChannelingStarted = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bEngineChannelingEnded = false;

	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_BriefingSelectionLocked();

	UFUNCTION()
	void OnRep_PhaseEndServerTime();

	UFUNCTION()
	void OnRep_ZoneScores();

	FTimerHandle PhaseUiUpdateTimerHandle;
};
