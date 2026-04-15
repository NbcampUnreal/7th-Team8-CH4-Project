#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HeistMessageSubsystem.generated.h"

class UHeistMessageSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHeistOnSoundDetected, FVector, OriginLocation, float, DetectionRadius);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeistOnFlashlightAlert, bool, bIsDetected);

UENUM(BlueprintType)
enum class EHeistMessageMatch : uint8
{
	ExactMatch,
	PartialMatch
};

struct HEIST_API FHeistMessageListenerHandle
{
public:
	FHeistMessageListenerHandle() = default;

	bool IsValid() const;
	void Unregister();

private:
	TWeakObjectPtr<UHeistMessageSubsystem> Subsystem;
	FGameplayTag Channel;
	uint32 ID = 0;

	friend class UHeistMessageSubsystem;

	FHeistMessageListenerHandle(UHeistMessageSubsystem* InSubsystem, FGameplayTag InChannel, uint32 InID);
};

UCLASS()
class HEIST_API UHeistMessageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UHeistMessageSubsystem& Get(const UObject* WorldContextObject);

	template <typename T>
	void BroadcastMessage(FGameplayTag Channel, const T& Message)
	{
		FInstancedStruct Payload;
		Payload.InitializeAs<T>(Message);
		BroadcastMessageInternal(Channel, Payload);
	}

	template <typename T>
	FHeistMessageListenerHandle RegisterListener(
		FGameplayTag Channel,
		TFunction<void(FGameplayTag, const T&)> Callback,
		EHeistMessageMatch MatchType = EHeistMessageMatch::ExactMatch)
	{
		const uint32 NewID = ++NextID;

		FListenerData NewListener;
		NewListener.ID = NewID;
		NewListener.Channel = Channel;
		NewListener.MatchType = MatchType;
		NewListener.StructType = T::StaticStruct();
		NewListener.Callback = [Callback](FGameplayTag Tag, const FInstancedStruct& Payload)
			{
				const T* Data = Payload.GetPtr<T>();
				if (Data != nullptr)
				{
					Callback(Tag, *Data);
				}
			};

		Listeners.Add(NewListener);
		return FHeistMessageListenerHandle(this, Channel, NewID);
	}

	void UnregisterListener(FHeistMessageListenerHandle& Handle);

	UPROPERTY(BlueprintAssignable, Category = "Heist|Messaging|UI")
	FHeistOnSoundDetected OnSoundDetectedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Heist|Messaging|UI")
	FHeistOnFlashlightAlert OnFlashlightAlertEvent;

private:
	struct FListenerData
	{
		uint32 ID = 0;
		FGameplayTag Channel;
		EHeistMessageMatch MatchType = EHeistMessageMatch::ExactMatch;
		const UScriptStruct* StructType = nullptr;
		TFunction<void(FGameplayTag, const FInstancedStruct&)> Callback;
	};

	void BroadcastMessageInternal(FGameplayTag Channel, const FInstancedStruct& Payload);
	bool DoesChannelMatch(FGameplayTag ListenerChannel, FGameplayTag BroadcastChannel, EHeistMessageMatch MatchType) const;

	TArray<FListenerData> Listeners;
	uint32 NextID = 0;

	bool bIsBroadcasting = false;
	TArray<uint32> PendingRemovals;

	FHeistMessageListenerHandle SoundDetectedBridgeHandle;
	FHeistMessageListenerHandle FlashlightAlertBridgeHandle;
};
