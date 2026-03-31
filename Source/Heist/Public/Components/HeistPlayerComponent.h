#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "HeistPlayerComponent.generated.h"

struct FInputActionValue;
struct FActorInitStateChangedParams;
class UInputComponent;
class UGameFrameworkComponentManager;

/**
 * 입력 바인딩을 담당하는 컴포넌트.
 * IGameFrameworkInitStateInterface를 구현하여 PawnExtensionComponent의
 * GameplayReady 도달을 감지하고, InputComponent가 준비되면 입력을 바인딩한다.
 *
 * 이동/시야 — 직접 핸들러 바인딩
 * 나머지   — ASC->AbilityInputTagPressed/Released 로 전달
 */
UCLASS()
class HEIST_API UHeistPlayerComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UHeistPlayerComponent(const FObjectInitializer& ObjectInitializer);

	static UHeistPlayerComponent* FindPlayerComponent(const AActor* Actor);

	static const FName NAME_ActorFeatureName;

	// IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// AHeistCharacter::SetupPlayerInputComponent에서 호출
	void OnPawnInputComponentReady(UInputComponent* InputComponent);

private:
	void BindInput();

	void HandleMoveInput(const FInputActionValue& Value);
	void HandleAbilityInputTagPressed(FGameplayTag InputTag);
	void HandleAbilityInputTagReleased(FGameplayTag InputTag);

	bool bInputComponentReady = false;
	bool bInputBound = false;

	TArray<uint32> AbilityInputBindHandles;
};
