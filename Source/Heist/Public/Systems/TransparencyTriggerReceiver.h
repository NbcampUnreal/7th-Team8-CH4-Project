#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TransparencyTriggerReceiver.generated.h"

class AActor;

UINTERFACE(BlueprintType)
class HEIST_API UTransparencyTriggerReceiver : public UInterface
{
	GENERATED_BODY()
};

class HEIST_API ITransparencyTriggerReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Heist|Transparency")
	void EvaluateTransparencyForActor(AActor* SourceActor);

	// 알파 덧입히기 (투명화 시작)
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Heist|Transparency")
	void ApplyTransparency(AActor* SourceActor);

	// 알파 벗기기 (투명화 종료/원상복구)
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Heist|Transparency")
	void RevertTransparency(AActor* SourceActor);
};
