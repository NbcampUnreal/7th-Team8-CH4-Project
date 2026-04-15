#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "DetectionOverlayWidget.generated.h"

struct FHeistSoundDetectedMessage;
struct FHeistFlashlightAlertMessage;

UCLASS()
class HEIST_API UDetectionOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Heist|UI")
	void OnSoundDetected(FVector OriginLocation, float DetectionRadius);

	UFUNCTION(BlueprintImplementableEvent, Category = "Heist|UI")
	void OnFlashlightAlertStateChanged(bool bIsDetected);

private:
	FHeistMessageListenerHandle SoundDetectedHandle;
	FHeistMessageListenerHandle FlashlightAlertHandle;

	void HandleSoundDetectedMessage(FGameplayTag Channel, const FHeistSoundDetectedMessage& Message);
	void HandleFlashlightAlertMessage(FGameplayTag Channel, const FHeistFlashlightAlertMessage& Message);
};
