#include "Systems/Messaging/HeistMessageSubsystem.h"

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

    if (bIsBroadcasting)
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
    bIsBroadcasting = true;

    for (const FListenerData& Listener : Listeners)
    {
        if (!DoesChannelMatch(Listener.Channel, Channel, Listener.MatchType)) continue;
        if (Listener.StructType != Payload.GetScriptStruct()) continue;

        Listener.Callback(Channel, Payload);
    }

    bIsBroadcasting = false;

    for (uint32 PendingID : PendingRemovals)
    {
        Listeners.RemoveAll([PendingID](const FListenerData& Entry)
        {
            return Entry.ID == PendingID;
        });
    }
    PendingRemovals.Reset();
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
