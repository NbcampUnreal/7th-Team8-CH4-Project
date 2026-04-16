#pragma once

#include "CoreMinimal.h"
#include "Core/HeistMatchTypes.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "HeistPlayerState.generated.h"

class UHeistAbilitySystemComponent;
class UHeistAttributeSet;
class UHeistBriefingPlayerComponent;
class UHeistVoipTalker;

/**
 * ASC를 소유한다. 리스폰 후에도 GAS 상태가 유지된다.
 */
UCLASS()
class HEIST_API AHeistPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AHeistPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UHeistAbilitySystemComponent* GetHeistAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "Heist|Briefing")
	EHeistTeam GetAssignedTeam() const { return AssignedTeam; }

	UFUNCTION(BlueprintPure, Category = "Heist|Briefing")
	bool IsThief() const { return AssignedTeam == EHeistTeam::Thief; }

	UFUNCTION(BlueprintPure, Category = "Heist|Briefing")
	bool IsPolice() const { return AssignedTeam == EHeistTeam::Police; }

	UFUNCTION(BlueprintPure, Category = "Voice")
	UHeistVoipTalker* GetVoipTalker() const;

	bool GetIsReady() const { return bIsReady; }
	void SetIsReady(bool bNewIsReady);

	bool GetIsHost() const { return bIsHost; }
	void SetIsHost(bool bNewIsHost);

	UHeistBriefingPlayerComponent* GetBriefingPlayerComponent() const { return BriefingPlayerComponent; }

	void SetAssignedTeam(EHeistTeam InTeam);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SeamlessTravelTo(APlayerState* NewPlayerState) override;
	virtual void OnSetUniqueId() override;
	virtual void OnRep_PlayerName() override;

private:
	void BindVoipTalker();
	void UnbindVoipTalker();

	UPROPERTY()
	TObjectPtr<UHeistAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UHeistAttributeSet> AttributeSet;

	UPROPERTY(VisibleDefaultsOnly, Category = "Voice", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistVoipTalker> VoipTalker;

	UPROPERTY(ReplicatedUsing = OnRep_bIsReady)
	bool bIsReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_bIsHost)
	bool bIsHost = false;

	UFUNCTION()
	void OnRep_bIsReady();

	UFUNCTION()
	void OnRep_bIsHost();

	void BroadcastPlayersChanged() const;
	void BroadcastReadyStateChanged() const;

	UPROPERTY(VisibleDefaultsOnly, Category = "Heist|Briefing", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistBriefingPlayerComponent> BriefingPlayerComponent;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Heist|Briefing", meta = (AllowPrivateAccess = "true"))
	EHeistTeam AssignedTeam = EHeistTeam::None;
};
