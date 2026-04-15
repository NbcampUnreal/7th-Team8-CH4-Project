
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
	void SetCurrentPhase(EHeistMatchPhase InPhase);
	void SetPhaseRemainingTime(float InRemainingTime);
	void SetBriefingSelectionLocked(bool bLocked);
	void SetPhaseEndServerTime(float InPhaseEndServerTime);

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
	
	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_BriefingSelectionLocked();

	UFUNCTION()
	void OnRep_PhaseEndServerTime();

	FTimerHandle PhaseUiUpdateTimerHandle;
};
