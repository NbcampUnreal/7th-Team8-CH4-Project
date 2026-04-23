#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Core/HeistMatchTypes.h"
#include "HeistBGMManagerComponent.generated.h"

class AThiefCharacter;
class APawn;
struct FHeistFlashlightAlertMessage;
struct FHeistPhaseChangedMessage;

UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UHeistBGMManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeistBGMManagerComponent();

	void BindToPawn(APawn* InPawn);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnPhaseChanged(FGameplayTag Channel, const FHeistPhaseChangedMessage& Message);
	void OnFlashlightAlertMessage(FGameplayTag Channel, const FHeistFlashlightAlertMessage& Message);

	UFUNCTION()
	void OnThiefSpottedByPolice(AThiefCharacter* SpottedThief);

	UFUNCTION()
	void OnThiefLostByPolice(AThiefCharacter* LostThief);

	void UpdateBGMState();
	void ClearPawnBindings();

	int32 ActiveChaseCount;
	EHeistMatchPhase CurrentMatchPhase;

	FHeistMessageListenerHandle PhaseChangeListenerHandle;
	FHeistMessageListenerHandle AlertMessageListenerHandle;
};
