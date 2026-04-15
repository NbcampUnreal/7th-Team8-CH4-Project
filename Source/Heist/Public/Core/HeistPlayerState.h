#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "HeistPlayerState.generated.h"

class UHeistAbilitySystemComponent;
class UHeistAttributeSet;
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

	UFUNCTION(BlueprintPure, Category = "Voice")
	UHeistVoipTalker* GetVoipTalker() const;

	bool GetIsReady() const { return bIsReady; }
	void SetIsReady(bool bNewIsReady);

	bool GetIsHost() const { return bIsHost; }
	void SetIsHost(bool bNewIsHost);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnSetUniqueId() override;
	virtual void OnRep_PlayerName() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
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
};
