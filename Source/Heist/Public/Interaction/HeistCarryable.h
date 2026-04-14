#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HeistCarryable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UHeistCarryable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class HEIST_API IHeistCarryable
{
	GENERATED_BODY()

public:
	//// 권장 운반 인원 반환 (RequiredCarriers)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Heist|Carry")
	int32 GetRequiredCarriers() const;

	//// 현재 운반 인원에 따른 이속 배율 반환 (CarrySpeedMultiplier 또는 SoloCarrySpeedMultiplier)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Heist|Carry")
	float GetCarrySpeedMultiplier(int32 CarrierCount) const;

	//// 단독 운반 시 적용할 소음 배율 (SoloCarryNoiseMultiplier)
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Heist|Carry")
	//float GetNoiseMultiplier(int32 CurrentCarrierCount) const;

	//// GPS 활성화 여부 (bTriggerGPS)
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Heist|Carry")
	//bool ShouldTriggerGPS() const;

	//// 경보 발동 여부 (bTriggerAlarm)
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Heist|Carry")
	//bool ShouldTriggerAlarm() const;
};
