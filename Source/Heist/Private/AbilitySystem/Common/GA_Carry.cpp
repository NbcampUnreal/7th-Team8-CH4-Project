#include "AbilitySystem/Common/GA_Carry.h"

#include "AbilitySystem/HeistTags_FlagTags.h"
#include "AbilitySystemComponent.h"
#include "Actors/ItemActor.h"
#include "Character/HeistCharacter.h"
#include "Character/HeistTags_State.h"
#include "AbilitySystem/HeistTags_Event.h"

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
		int32 Carriers = Item->GetRequiredCarriers();
		if (Carriers == 1)
		{
			SpeedMult = Item->GetCarrySpeedMultiplier();
			Item->OnPickedUp(Carrier);
		}
		else
		{
			//TODO : 2인 이상 물건 로직 구현
		}
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(ASC)) return;

	if (IsValid(CarryEffect))
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(CarryEffect, 1.0f, EffectContext);
		FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(FName("Data.CarrySpeedMultiplier"));
		EffectSpec.Data.Get()->SetSetByCallerMagnitude(DataTag, SpeedMult);

		CarryEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	}
}

void UGA_Carry::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogTemp, Warning, TEXT("EndAbility Called!"));
	if (CarryEffectHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(CarryEffectHandle);
		CarryEffectHandle.Invalidate();
	}

	AHeistCharacter* Carrier = Cast<AHeistCharacter>(GetAvatarActorFromActorInfo());
	Item->OnDropOff(Carrier);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
