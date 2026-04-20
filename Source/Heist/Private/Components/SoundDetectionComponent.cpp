#include "Components/SoundDetectionComponent.h"

#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "AbilitySystem/HeistTags_Event.h"

#include "GameFramework/Pawn.h"

USoundDetectionComponent::USoundDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USoundDetectionComponent::ReceiveSoundDetection(const FVector& SoundLocation, float DetectionRadius)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	if (OwnerPawn->IsLocallyControlled())
	{
		BroadcastToUI(SoundLocation, DetectionRadius);
	}
	else if (OwnerPawn->HasAuthority())
	{
		Client_ReceiveSoundDetection(SoundLocation, DetectionRadius);
	}
}

void USoundDetectionComponent::Client_ReceiveSoundDetection_Implementation(const FVector& SoundLocation, float DetectionRadius)
{
	BroadcastToUI(SoundLocation, DetectionRadius);
}

void USoundDetectionComponent::BroadcastToUI(const FVector& SoundLocation, float DetectionRadius)
{
	FHeistSoundDetectedMessage Message;
	Message.OriginLocation = SoundLocation;
	Message.DetectionRadius = DetectionRadius;

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(GetWorld());
	MessageSubsystem.BroadcastMessage(HeistMessageTags::Message_UI_SoundDetected, Message);
}
