#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ThiefEscortComponent.generated.h"

class AHeistCharacter;

UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UThiefEscortComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UThiefEscortComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetEscortedBy(AHeistCharacter* InPolice);

	UFUNCTION(BlueprintCallable, Category = "Heist|Escort")
	AHeistCharacter* GetEscortedBy() const { return EscortedBy; }

private:
	// 현재 나를 이송 중인 경찰
	UPROPERTY(ReplicatedUsing = OnRep_EscortedBy)
	TObjectPtr<AHeistCharacter> EscortedBy;

	UFUNCTION()
	void OnRep_EscortedBy();

	// 경찰 후방 동기화 거리
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float EscortOffsetDistance = 80.0f;
};
