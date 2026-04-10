#include "AbilitySystem/Common/GA_Carry.h"

#include "AbilitySystem/HeistTags_FlagTags.h"
#include "AbilitySystemComponent.h"
#include "Actors/ItemActor.h"
#include "Character/HeistCharacter.h"

void UGA_Carry::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(ASC)) return;

	ASC->AddReplicatedLooseGameplayTag(HeistFlagTags::Tag_Carrying);

	float SpeedMult = 1.0f;
			
	AHeistCharacter* Carrier = Cast<AHeistCharacter>(GetAvatarActorFromActorInfo());
	if (TriggerEventData)
	{
		TArray<TWeakObjectPtr<AActor>> TargetActors = TriggerEventData->TargetData.Get(0)->GetActors();

		if (TargetActors.Num() > 0)
		{
			AActor* TargetActor = TargetActors[0].Get();
			AItemActor* Item = Cast<AItemActor>(TargetActor);
			int32 Carriers = Item->GetRequiredCarriers();
			if (Carriers == 1)
			{
				SpeedMult = Item->GetCarrySpeedMultiplier();
				Item->OnPickedUp(Carrier, FName("hand_r"));
			}
			else
			{
				//TODO : 2인 이상 물건 로직 구현
			}
		}
	}

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
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (IsValid(ASC))
	{
		ASC->RemoveReplicatedLooseGameplayTag(HeistFlagTags::Tag_Carrying);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
