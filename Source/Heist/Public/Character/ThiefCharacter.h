#pragma once

#include "CoreMinimal.h"
#include "Character/HeistCharacter.h"
#include "ThiefCharacter.generated.h"

class UHeistInteractSphereComponent;
class UHeistTransparencyComponent;
class UThiefEscortComponent;
class UHeistNoiseComponent;
class UHeistZoneComponent;
class UPointLightComponent;
class UFlashlightDetectionComponent;
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

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void UnPossessed() override;

	bool CheckCanInteract(ACharacter* Interactor) const;
	FGameplayTag ResolveInteractAbilityTag(ACharacter* Interactor) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistInteractSphereComponent> InteractSphereComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistTransparencyComponent> TransparencyComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UThiefEscortComponent> EscortComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistNoiseComponent> NoiseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistZoneComponent> ZoneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Light", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> CloseVisionPointLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Heist|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFlashlightDetectionComponent> FlashlightDetectionComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Light")
	float CloseVisionIntensity = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Light")
	float CloseVisionRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Movement")
	float MaxRotationRateYaw = 150.f;

	FDelegateHandle ZoneTagListenerHandle;

	void OnZoneTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
};
