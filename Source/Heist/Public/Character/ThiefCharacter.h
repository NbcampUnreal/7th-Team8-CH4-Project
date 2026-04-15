#pragma once

#include "CoreMinimal.h"
#include "Character/HeistCharacter.h"
#include "ThiefCharacter.generated.h"

class UHeistInteractSphereComponent;
class UThiefEscortComponent;
class UHeistNoiseComponent;
/**
 * 도둑 전용 캐릭터.
 * 게임플레이 로직은 GA와 컴포넌트에 위임한다.
 */
UCLASS()
class HEIST_API AThiefCharacter : public AHeistCharacter
{
	GENERATED_BODY()

public:
	AThiefCharacter(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Heist|Components")
	UThiefEscortComponent* GetThiefEscortComponent() const { return EscortComponent; }

	UFUNCTION(BlueprintCallable, Category = "Heist|Components")
	UHeistNoiseComponent* GetHeistNoiseComponent() const { return NoiseComponent; }

	UFUNCTION(BlueprintCallable, Category = "Heist|Audio")
	void ReportFootstep();

protected:
	virtual void BeginPlay() override;

	bool CheckCanInteract(ACharacter* Interactor) const;
	FGameplayTag ResolveInteractAbilityTag(ACharacter* Interactor) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UThiefEscortComponent> EscortComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistInteractSphereComponent> InteractSphereComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistNoiseComponent> NoiseComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Movement")
	float MaxRotationRateYaw = 150.f;
};
