#pragma once

#include "CoreMinimal.h"
#include "Character/HeistCharacter.h"
#include "PoliceCharacter.generated.h"

class UFlashlightComponent;
class UThiefEscortComponent;
class USoundDetectionComponent;
class USpotLightComponent;
class UHeistZoneComponent;
class UPointLightComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFlashlightComponent> FlashlightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundDetectionComponent> SoundDetectionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistZoneComponent> ZoneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Light", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpotLightComponent> FlashlightSpotLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Light", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> CloseVisionPointLight;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Light")
	float FlashlightIntensity = 30000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Light")
	float FlashlightRadius = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Light")
	float FlashlightInnerConeAngle = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Light")
	float FlashlightOuterConeAngle = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Light")
	float CloseVisionIntensity = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Light")
	float CloseVisionRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Movement")
	float MaxRotationRateYaw = 200.f;

	FDelegateHandle ZoneTagListenerHandle;
	FTimerHandle ASCBindTimerHandle;

	void TryBindZoneTagListener();
	void OnZoneTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
};
