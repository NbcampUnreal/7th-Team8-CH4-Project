#pragma once

#include "CoreMinimal.h"
#include "Character/HeistCharacter.h"
#include "PoliceCharacter.generated.h"

class UFlashlightComponent;
class UThiefEscortComponent;
class USoundDetectionComponent;

/**
 * 경찰 전용 캐릭터.
 * 게임플레이 로직은 GA와 컴포넌트에 위임한다.
 */
UCLASS()
class HEIST_API APoliceCharacter : public AHeistCharacter
{
	GENERATED_BODY()

public:
	APoliceCharacter(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Heist|Components")
	UThiefEscortComponent* GetThiefEscortComponent() const { return EscortComponent; }

	UFUNCTION(BlueprintCallable, Category = "Heist|Components")
	UFlashlightComponent* GetFlashlightComponent() const { return FlashlightComponent; }

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void UnPossessed() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UThiefEscortComponent> EscortComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Movement")
	float MaxRotationRateYaw = 200.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFlashlightComponent> FlashlightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundDetectionComponent> SoundDetectionComponent;
};
