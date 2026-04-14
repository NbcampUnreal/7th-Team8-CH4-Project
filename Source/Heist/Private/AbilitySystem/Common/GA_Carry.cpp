#include "AbilitySystem/Common/GA_Carry.h"

#include "AbilitySystem/HeistTags_FlagTags.h"
#include "AbilitySystemComponent.h"
#include "Actors/ItemActor.h"
#include "Character/HeistCharacter.h"
#include "Character/HeistTags_State.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

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

	float SpeedMult = 1.0f;
			
	AHeistCharacter* Carrier = Cast<AHeistCharacter>(GetAvatarActorFromActorInfo());
	if (TriggerEventData && TriggerEventData->Target)
	{
		AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
		Item = Cast<AItemActor>(TargetActor);
		Item->OnPickedUp(Carrier);
	}

	UpdateCarryEffect();

	UAbilityTask_WaitGameplayEvent* WaitUpdateTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,	FGameplayTag::RequestGameplayTag(TEXT("Event.CarryUpdate")), nullptr, false, false);
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
	Item->OnDropOff(Carrier);

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

	int32 CurrentCarriers = Item->GetCurrentCarrierCount();
	float SpeedMult = AItemActor::Execute_GetCarrySpeedMultiplier(Item, CurrentCarriers);

	if (IsValid(CarryEffect))
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(CarryEffect, 1.0f, EffectContext);
		FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(FName("Data.CarrySpeedMultiplier"));
		EffectSpec.Data.Get()->SetSetByCallerMagnitude(DataTag, SpeedMult);

		CarryEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	}
}

void UGA_Carry::OnCarryDropEventReceived(FGameplayEventData Payload)
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}
