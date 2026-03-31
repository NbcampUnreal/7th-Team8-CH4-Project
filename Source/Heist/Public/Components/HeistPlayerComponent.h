#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "HeistPlayerComponent.generated.h"

struct FInputActionValue;
class UInputComponent;

/**
 * 입력 바인딩을 담당하는 컴포넌트.
 * PawnExtensionComponent의 OnGameplayReady 델리게이트를 구독하고,
 * SetupPlayerInputComponent 완료 후 두 조건이 모두 충족되면 입력을 바인딩한다.
 *
 * 이동/시야 — 직접 핸들러 바인딩
 * 나머지   — ASC->AbilityInputTagPressed/Released 로 전달
 */
UCLASS()
class HEIST_API UHeistPlayerComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UHeistPlayerComponent(const FObjectInitializer& ObjectInitializer);

	static UHeistPlayerComponent* FindPlayerComponent(const AActor* Actor);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// AHeistCharacter::SetupPlayerInputComponent에서 호출
	void OnPawnInputComponentReady(UInputComponent* InputComponent);

private:
	void OnGameplayReady();
	void TryBindInput();
	void BindInput();

	// 이동 핸들러
	void HandleMoveInput(const FInputActionValue& Value);

	// 어빌리티 입력 핸들러 — 태그를 직접 받아 ASC로 전달
	void HandleAbilityInputTagPressed(FGameplayTag InputTag);
	void HandleAbilityInputTagReleased(FGameplayTag InputTag);

	bool bInputComponentReady = false;
	bool bGameplayReady       = false;
	bool bInputBound          = false;

	TArray<uint32> AbilityInputBindHandles;
};
