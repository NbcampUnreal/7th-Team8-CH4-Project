#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "HeistCharacter.generated.h"

class UHeistPawnExtensionComponent;
class UHeistPlayerComponent;
class UHeistAbilitySystemComponent;
class UHeistPawnData;
class USpringArmComponent;
class UCameraComponent;

/**
 * 컴포넌트 소유 + ASC 캐싱만 담당하는 얇은 껍데기.
 * 게임플레이 로직은 컴포넌트와 GA에 위임한다.
 *
 * 소유 컴포넌트:
 *   UHeistPawnExtensionComponent — GAS 초기화, InitState 머신
 *   UHeistPlayerComponent        — 입력 바인딩 (GameplayReady 이후)
 *
 * TODO: ASC를 PlayerState로 이동 예정 (리스폰 대응)
 */
UCLASS()
class HEIST_API AHeistCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AHeistCharacter(const FObjectInitializer& ObjectInitializer);

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UHeistAbilitySystemComponent* GetHeistAbilitySystemComponent() const;

protected:
	virtual void BeginPlay() override;

	// [SERVER]
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	// [CLIENT] 자율 프록시 초기화
	virtual void OnRep_Controller() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void InitializeGameplayAbilitySystem();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heist|Pawn", meta = (AllowPrivateAccess = true))
	TObjectPtr<UHeistPawnData> DefaultPawnData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = true))
	TObjectPtr<UHeistPawnExtensionComponent> PawnExtensionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = true))
	TObjectPtr<UHeistPlayerComponent> PlayerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Camera", meta = (AllowPrivateAccess = true))
	TObjectPtr<USpringArmComponent> CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Camera", meta = (AllowPrivateAccess = true))
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY()
	TObjectPtr<UHeistAbilitySystemComponent> AbilitySystemComponent;
};
