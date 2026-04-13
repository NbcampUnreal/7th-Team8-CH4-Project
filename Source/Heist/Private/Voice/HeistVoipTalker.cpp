#include "Voice/HeistVoipTalker.h"

#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "GameFramework/PlayerState.h"
#include "Components/AudioComponent.h"

void UHeistVoipTalker::OnTalkingBegin(UAudioComponent* AudioComponent)
{
	APlayerState* OwnerPS = GetOwner<APlayerState>();

	if (IsValid(OwnerPS) && IsValid(OwnerPS->GetPawn()))
	{
		Settings.ComponentToAttachTo = OwnerPS->GetPawn()->GetRootComponent();
		AudioComponent->AttachToComponent(
			OwnerPS->GetPawn()->GetRootComponent(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	if (IsValid(Settings.AttenuationSettings))
	{
		AudioComponent->bAllowSpatialization = true;
		AudioComponent->SetAttenuationSettings(Settings.AttenuationSettings);
	}

	Super::OnTalkingBegin(AudioComponent);
	BroadcastTalkingState(true);
}

void UHeistVoipTalker::OnTalkingEnd()
{
	Super::OnTalkingEnd();
	BroadcastTalkingState(false);
}

void UHeistVoipTalker::BroadcastTalkingState(bool bIsTalking)
{
	APlayerState* OwnerPS = GetOwner<APlayerState>();
	if (!IsValid(OwnerPS)) return;

	FHeistVoiceTalkingStateMessage Message;
	Message.PlayerState = OwnerPS;
	Message.bIsTalking = bIsTalking;

	UHeistMessageSubsystem::Get(this).BroadcastMessage(
		HeistMessageTags::Message_Voice_TalkingStateChanged, Message);
}
