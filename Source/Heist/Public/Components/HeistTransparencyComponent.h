#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeistTransparencyComponent.generated.h"

class UMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UHeistTransparencyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeistTransparencyComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetTargetVisibility(bool bVisible);
	// [로비 전용] 경찰 프리뷰 → 도둑 전환 시 투명도 상태를 완전 초기화한다.
	void ForceRestoreVisibility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Heist|Transparency")
	bool bStartInvisibleToPolice = true;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Transparency")
	float FadeSpeed = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Transparency")
	FName OpacityParamName = TEXT("OpacityAlpha");

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Transparency")
	float OpacityNearlyEqualTolerance = 0.01f;

	UPROPERTY()
	TArray<TObjectPtr<UMeshComponent>> CachedMeshComponents;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	bool bCachedIsLocalViewerPolice = false;
	bool bHasCachedViewer = false;
	bool bIsTargetVisible;

	float CurrentOpacity;

	bool IsLocalViewerPolice();
	void SetAllMeshVisibility(bool bVisible);
};
