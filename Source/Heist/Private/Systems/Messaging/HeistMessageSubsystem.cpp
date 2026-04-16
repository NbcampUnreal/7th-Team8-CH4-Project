#include "Systems/Messaging/HeistMessageSubsystem.h"

#include "Systems/Messaging/HeistTags_Message.h"
#include "Systems/Messaging/HeistMessageTypes.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

FHeistMessageListenerHandle::FHeistMessageListenerHandle(UHeistMessageSubsystem* InSubsystem, FGameplayTag InChannel, uint32 InID)
	: Subsystem(InSubsystem), Channel(InChannel), ID(InID)
{
}

bool FHeistMessageListenerHandle::IsValid() const
{
	return ID != 0 && Subsystem.IsValid();
}

void FHeistMessageListenerHandle::Unregister()
{
	if (UHeistMessageSubsystem* SubsystemPtr = Subsystem.Get())
	{
		SubsystemPtr->UnregisterListener(*this);
	}
}

void UHeistMessageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 1. 소리 감지 메시지 브릿지 연동
	SoundDetectedBridgeHandle = RegisterListener<FHeistSoundDetectedMessage>(
		HeistMessageTags::Message_UI_SoundDetected,
		[this](FGameplayTag Channel, const FHeistSoundDetectedMessage& Message)
		{
			OnSoundDetectedEvent.Broadcast(Message.OriginLocation, Message.DetectionRadius);
		}
	);

	// 2. 플래시라이트 경고 메시지 브릿지 연동
	FlashlightAlertBridgeHandle = RegisterListener<FHeistFlashlightAlertMessage>(
		HeistMessageTags::Message_UI_FlashlightAlert,
		[this](FGameplayTag Channel, const FHeistFlashlightAlertMessage& Message)
		{
			OnFlashlightAlertEvent.Broadcast(Message.bIsDetected);
		}
	);
}

void UHeistMessageSubsystem::Deinitialize()
{
	if (SoundDetectedBridgeHandle.IsValid())
	{
		SoundDetectedBridgeHandle.Unregister();
	}

	if (FlashlightAlertBridgeHandle.IsValid())
	{
		FlashlightAlertBridgeHandle.Unregister();
	}

	Super::Deinitialize();
}

UHeistMessageSubsystem& UHeistMessageSubsystem::Get(const UObject* WorldContextObject)
{
	UGameInstance* GameInstance = WorldContextObject->GetWorld()->GetGameInstance();
	check(IsValid(GameInstance));

	UHeistMessageSubsystem* Subsystem = GameInstance->GetSubsystem<UHeistMessageSubsystem>();
	check(IsValid(Subsystem));

	return *Subsystem;
}


void UHeistMessageSubsystem::UnregisterListener(FHeistMessageListenerHandle& Handle)
{
	if (!Handle.IsValid()) return;

	if (BroadcastDepth > 0)
	{
		PendingRemovals.Add(Handle.ID);
	}
	else
	{
		Listeners.RemoveAll([ID = Handle.ID](const FListenerData& Entry)
			{
				return Entry.ID == ID;
			});
	}

	Handle.ID = 0;
	Handle.Subsystem = nullptr;
}

void UHeistMessageSubsystem::BroadcastMessageInternal(FGameplayTag Channel, const FInstancedStruct& Payload)
{
	BroadcastDepth++;

	for (const FListenerData& Listener : Listeners)
	{
		if (!DoesChannelMatch(Listener.Channel, Channel, Listener.MatchType)) continue;
		if (Listener.StructType != Payload.GetScriptStruct()) continue;

		Listener.Callback(Channel, Payload);
	}

	BroadcastDepth--;

	if (BroadcastDepth == 0)
	{
		for (uint32 PendingID : PendingRemovals)
		{
			Listeners.RemoveAll([PendingID](const FListenerData& Entry)
				{
					return Entry.ID == PendingID;
				});
		}
		PendingRemovals.Reset();

		for (FListenerData& Addition : PendingAdditions)
		{
			Listeners.Add(MoveTemp(Addition));
		}
		PendingAdditions.Reset();
	}
}

bool UHeistMessageSubsystem::DoesChannelMatch(FGameplayTag ListenerChannel, FGameplayTag BroadcastChannel, EHeistMessageMatch MatchType) const
{
	if (MatchType == EHeistMessageMatch::ExactMatch)
	{
		return ListenerChannel == BroadcastChannel;
	}

	// PartialMatch: "Message.Ping" 리스너가 "Message.Ping.Danger" 브로드캐스트를 수신
	return BroadcastChannel.MatchesTag(ListenerChannel);
}
