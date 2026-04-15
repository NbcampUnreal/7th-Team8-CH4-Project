#include "UI/DetectionOverlayWidget.h"

#include "Systems/Messaging/HeistTags_Message.h"
#include "Systems/Messaging/HeistMessageTypes.h"

void UDetectionOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(GetWorld());

	SoundDetectedHandle = MessageSubsystem.RegisterListener<FHeistSoundDetectedMessage>(
		HeistMessageTags::Message_UI_SoundDetected,
		[this](FGameplayTag Channel, const FHeistSoundDetectedMessage& Message)
		{
			HandleSoundDetectedMessage(Channel, Message);
		}
	);

	FlashlightAlertHandle = MessageSubsystem.RegisterListener<FHeistFlashlightAlertMessage>(
		HeistMessageTags::Message_UI_FlashlightAlert,
		[this](FGameplayTag Channel, const FHeistFlashlightAlertMessage& Message)
		{
			HandleFlashlightAlertMessage(Channel, Message);
		}
	);
}

void UDetectionOverlayWidget::NativeDestruct()
{
	SoundDetectedHandle.Unregister();
	FlashlightAlertHandle.Unregister();

	Super::NativeDestruct();
}

void UDetectionOverlayWidget::HandleSoundDetectedMessage(FGameplayTag Channel, const FHeistSoundDetectedMessage& Message)
{
	OnSoundDetected(Message.OriginLocation, Message.DetectionRadius);
}

void UDetectionOverlayWidget::HandleFlashlightAlertMessage(FGameplayTag Channel, const FHeistFlashlightAlertMessage& Message)
{
	OnFlashlightAlertStateChanged(Message.bIsDetected);
}
