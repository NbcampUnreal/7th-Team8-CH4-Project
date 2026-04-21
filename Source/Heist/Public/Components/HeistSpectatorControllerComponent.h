#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistSpectatorControllerComponent.generated.h"

class AHeistPlayerController;
class AHeistPlayerState;
class UInputComponent;

/**
 * 관전자 대상 탐색과 시점 전환만 담당하는 PlayerController 보조 컴포넌트.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistSpectatorControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeistSpectatorControllerComponent();

	void BindInput(UInputComponent* InputComponent);
	void EnterArrestSpectating();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void SpectateNextTarget();
	void SpectatePreviousTarget();
	void SpectateInDirection(int32 Direction);
	void ApplyViewTarget(AActor* Target);

	AHeistPlayerController* GetHeistPlayerController() const;
	void GatherSpectateCandidates(TArray<AActor*>& OutCandidates) const;
	AActor* FindFirstSpectateTarget() const;
	AActor* FindSpectateTargetFromCurrent(int32 Direction) const;
	bool IsCurrentViewTargetValid() const;

	// 일정상 raw key bind를 유지하되, 추후 EnhancedInput으로 전환할 수 있게 키만 노출한다.
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Spectator")
	FKey PreviousSpectateKey = EKeys::Q;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Spectator")
	FKey NextSpectateKey = EKeys::E;

	TWeakObjectPtr<AActor> CurrentViewTarget;
	bool bInputBound = false;
	bool bArrestSpectatingActive = false;
};
