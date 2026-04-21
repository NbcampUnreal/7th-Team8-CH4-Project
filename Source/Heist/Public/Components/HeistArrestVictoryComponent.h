#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistArrestVictoryComponent.generated.h"

class AHeistPlayerState;
class AThiefCharacter;

DECLARE_MULTICAST_DELEGATE(FOnPoliceVictory);

/**
 * Execution 중 도둑 전원 체포 여부를 추적하고 경찰 승리를 통지한다.
 * HeistMatchGameMode가 소유한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistArrestVictoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void RegisterAllThieves();
	void NotifyThiefArrested(AThiefCharacter* ArrestedThief);
	void NotifyThiefDisconnected(AHeistPlayerState* DisconnectedPS);

	FOnPoliceVictory OnPoliceVictory;

private:
	void TransitionToSpectator(AThiefCharacter* Thief);
	void CheckAllArrested();

	TArray<TWeakObjectPtr<AHeistPlayerState>> RegisteredThieves;
	TSet<TWeakObjectPtr<AHeistPlayerState>> ArrestedThieves;
};
