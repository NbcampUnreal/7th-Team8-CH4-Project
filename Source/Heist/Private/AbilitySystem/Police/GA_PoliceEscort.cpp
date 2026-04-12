
#include "AbilitySystem/Police/GA_PoliceEscort.h"

#include "Character/ThiefCharacter.h"
#include "Character/HeistTags_State.h"
#include "Components/ThiefEscortComponent.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "AbilitySystemComponent.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_PoliceEscort::UGA_PoliceEscort()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnGameplayEvent; // 게임 이벤트 트리거로 실행(상호작용)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
		
	ActivationOwnedTags.AddTag(HeistStateTags::State_Police_Escorting);   // 어빌리티 활성 동안 오너에게 Escorting 부착
	ActivationBlockedTags.AddTag(HeistStateTags::State_Police_Escorting); // 해당 태그 보유시 발동 차단
	
	CancelAbilitiesWithTag.AddTag(HeistStateTags::State_Stunned); // 이송중엔 채널링이 없으므로 취소 태그를 달아준다 - 추후 Kick 구현시 제거 가능

	AbilityTags.AddTag(HeistAbilityTags::Ability_Police_Escort);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = HeistAbilityTags::Ability_Police_Escort;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_PoliceEscort::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

	TargetThief = nullptr;

	// 상호작용 처리
	AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
	TargetThief = Cast<AThiefCharacter>(TargetActor);
	
	AHeistCharacter* Police = Cast<AHeistCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(TargetThief) || !IsValid(Police))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (HasAuthority(&CurrentActivationInfo))
	{
		UThiefEscortComponent* EscortComp = TargetThief->GetThiefEscortComponent();
		UAbilitySystemComponent* PoliceASC = GetAbilitySystemComponentFromActorInfo();
		
		if (!IsValid(EscortComp) || !IsValid(PoliceASC))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
		
		const bool bEscortStarted = EscortComp->BeginEscort(Police, EscortedEffectClass, PoliceASC);
		if (!bEscortStarted)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}
	
	//TODO(하민): 경찰에게 이속 40% 감소 GE 적용 -> 보원: Escorting에 40퍼 감소를, Escorted에 이동 차단을 넣죠!
	if (IsValid(EscortingEffectClass))
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(EscortingEffectClass, 1.f);
		if (Spec.IsValid() && Spec.Data.IsValid())
		{
			EscortingEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
		}
	}
	
	UAbilityTask_WaitGameplayEvent* WaitCarEvent = 
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HeistEventTags::Event_ArrivedAtCar);
	WaitCarEvent->EventReceived.AddDynamic(this, &UGA_PoliceEscort::OnArrivedAtCar);
	WaitCarEvent->ReadyForActivation();
}

void UGA_PoliceEscort::OnArrivedAtCar(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PoliceEscort::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (EscortingEffectHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(EscortingEffectHandle);
		EscortingEffectHandle.Invalidate();
	}
	
	AHeistCharacter* PoliceActor = Cast<AHeistCharacter>(GetAvatarActorFromActorInfo());
	if (HasAuthority(&CurrentActivationInfo) && IsValid(TargetThief))
	{
		UThiefEscortComponent* EscortComp = TargetThief->GetThiefEscortComponent();
		if (IsValid(EscortComp) && IsValid(PoliceActor) && EscortComp->IsEscortedBy(PoliceActor))
		{
			// 기존 취소 정책
			// bWasCancelled == true 이면 cuffed 유지
			EscortComp->InterruptEscort(
				CuffedEffectClass, 
				GetAbilitySystemComponentFromActorInfo(),
				/* bConvertedToCuffed */ bWasCancelled);
		}
		
		if (bWasCancelled)
		{
			if (IsValid(PoliceActor))
			{
				TargetThief->AddActorWorldOffset(PoliceActor->GetActorRightVector() * EscortReleaseOffset, true);
			}
		}
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
};
