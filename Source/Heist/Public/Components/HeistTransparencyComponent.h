#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistTransparencyComponent.generated.h"


UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UHeistTransparencyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeistTransparencyComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetTargetVisibility(bool bVisible);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Transparency")
	float FadeSpeed = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Transparency")
	FName OpacityParamName = TEXT("OpacityAlpha");

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

	bool bIsTargetVisible;
	float CurrentOpacity;
};
