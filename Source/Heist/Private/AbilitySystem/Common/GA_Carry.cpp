#include "AbilitySystem/Common/GA_Carry.h"

#include "AbilitySystem/HeistTags_FlagTags.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "Actors/ItemActor.h"
#include "Character/ThiefCharacter.h"
#include "Components/HeistNoiseComponent.h"	
#include "Systems/Audio/HeistAudioSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"

UGA_Carry::UGA_Carry()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnGameplayEvent; // 게임 이벤트 트리거로 실행(상호작용)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	AbilityTags.AddTag(HeistFlagTags::Tag_Carrying);

	ActivationOwnedTags.AddTag(HeistFlagTags::Tag_Carrying);
	ActivationBlockedTags.AddTag(HeistFlagTags::Tag_Carrying);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = HeistEventTags::Event_CarryStarted;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Carry::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AHeistCharacter* Carrier = Cast<AHeistCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Carrier))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (TriggerEventData != nullptr && IsValid(TriggerEventData->Target))
	{
		AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
		Item = Cast<AItemActor>(TargetActor);
		if (!IsValid(Item))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
		Item->OnPickedUp(Carrier);

		AThiefCharacter* Thief = Cast<AThiefCharacter>(Carrier);
		if (IsValid(Thief))
		{
			UHeistNoiseComponent* NoiseComp = Thief->GetHeistNoiseComponent();
			if (IsValid(NoiseComp))
			{
				NoiseComp->StartChannelingNoise(EHeistSoundType::Carry);
			}

			UWorld* World = GetWorld();
			if (IsValid(World) && World->GetNetMode() != NM_DedicatedServer)
			{
				UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
				if (IsValid(AudioSubsystem))
				{
					CarryAudioComp = AudioSubsystem->PlayLoopingSound(EHeistSoundType::Carry, Thief->GetRootComponent());
				}
			}
		}
	}

	UpdateCarryEffect();

	UAbilityTask_WaitGameplayEvent* WaitUpdateTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(TEXT("Event.CarryUpdate")), nullptr, false, false);
	WaitUpdateTask->EventReceived.AddDynamic(this, &UGA_Carry::OnCarryUpdateEventReceived);
	WaitUpdateTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitDropTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(TEXT("Event.CarryDrop")), nullptr, false, false);
	WaitDropTask->EventReceived.AddDynamic(this, &UGA_Carry::OnCarryDropEventReceived);
	WaitDropTask->ReadyForActivation();
}

void UGA_Carry::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (CarryEffectHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(CarryEffectHandle);
		CarryEffectHandle.Invalidate();
	}

	AHeistCharacter* Carrier = Cast<AHeistCharacter>(GetAvatarActorFromActorInfo());
	if (IsValid(Item) && IsValid(Carrier))
	{
		Item->OnDropOff(Carrier);
	}

	AThiefCharacter* Thief = Cast<AThiefCharacter>(Carrier);
	if (IsValid(Thief))
	{
		UHeistNoiseComponent* NoiseComp = Thief->GetHeistNoiseComponent();
		if (IsValid(NoiseComp))
		{
			NoiseComp->StopChannelingNoise();
			NoiseComp->MakeHeistNoise(EHeistSoundType::ItemDrop, Thief->GetActorLocation());
		}

		UWorld* World = GetWorld();
		if (IsValid(World) && World->GetNetMode() != NM_DedicatedServer)
		{
			UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
			if (IsValid(AudioSubsystem))
			{
				if (IsValid(CarryAudioComp))
				{
					AudioSubsystem->StopLoopingSound(CarryAudioComp);
					CarryAudioComp = nullptr;
				}

				AudioSubsystem->PlayOneShotSound(EHeistSoundType::ItemDrop, Thief->GetActorLocation());
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Carry::OnCarryUpdateEventReceived(FGameplayEventData Payload)
{
	UpdateCarryEffect();
}

void UGA_Carry::UpdateCarryEffect()
{
	if (!IsValid(Item)) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC)) return;

	if (CarryEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(CarryEffectHandle);
		CarryEffectHandle.Invalidate();
	}

	const int32 CurrentCarriers = Item->GetCurrentCarrierCount();
	const float SpeedMult = AItemActor::Execute_GetCarrySpeedMultiplier(Item, CurrentCarriers);

	if (IsValid(CarryEffect))
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(CarryEffect, 1.0f, EffectContext);
		FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(FName("Data.CarrySpeedMultiplier"));
		FGameplayEffectSpec* EffectSpecData = EffectSpec.Data.Get();
		if (EffectSpecData == nullptr) return;
		EffectSpecData->SetSetByCallerMagnitude(DataTag, SpeedMult);
		CarryEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecData);
	}
}

void UGA_Carry::OnCarryDropEventReceived(FGameplayEventData Payload)
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}
