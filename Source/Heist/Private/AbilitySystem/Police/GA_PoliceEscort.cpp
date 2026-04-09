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

	if (!IsValid(TargetThief))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 도둑의 이동 차단 - GE로 대체할게요, 캐릭터에 RegisterGameplayTagEvent로 이것과 똑같은 구현으로 옮겼습니다
	// UCharacterMovementComponent* ThiefMovement = TargetThief->GetCharacterMovement();
	// if (IsValid(ThiefMovement))
	// {
	// 	ThiefMovement->SetMovementMode(MOVE_None);
	// }
	
	// 도둑 ActionDisabled + 이동 차단 GE 부착 / 위치 추종 시작 — 서버에서만 처리
	if (HasAuthority(&CurrentActivationInfo))
	{
		UAbilitySystemComponent* TargetASC = TargetThief->GetAbilitySystemComponent();
		if (IsValid(TargetASC) && IsValid(EscortedEffectClass))
		{
			// SpecHandle은 이렇게 명세를 저장한다, GE로 사용될 SubclassOf를 받음
			FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(EscortedEffectClass, 1.f);
			// Spec을 실행하고 받은 Effect Handle이 캐싱된다.
			EscortedEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	    // 2. 위치 추종 시작
		UThiefEscortComponent* EscortComp = TargetThief->GetThiefEscortComponent();
		if (IsValid(EscortComp))
		{
			EscortComp->SetEscortedBy(Cast<AHeistCharacter>(GetAvatarActorFromActorInfo()));
		}
	}

	//TODO(하민): 경찰에게 이속 40% 감소 GE 적용 -> 보원: Escorting에 40퍼 감소를, Escorted에 이동 차단을 넣죠!
	if (IsValid(EscortingEffectClass))
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(EscortingEffectClass, 1.f);
		EscortingEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
	
	UAbilityTask_WaitGameplayEvent* WaitCarEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HeistEventTags::Event_ArrivedAtCar);
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
	if (IsValid(TargetThief))
	{
		// Escorting GE(속도저하) 제거 (이동 복구는 GE 제거가 자동 처리)
		if (EscortingEffectHandle.IsValid())
		{
			GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(EscortingEffectHandle);
			EscortingEffectHandle.Invalidate();
		}
		
		// 도둑 GE 해제 / 위치 추종 해제 / 밀어내기 — 서버에서만 처리
		if (HasAuthority(&CurrentActivationInfo))
		{
			UAbilitySystemComponent* TargetASC = TargetThief->GetAbilitySystemComponent();
			if (IsValid(TargetASC) && EscortedEffectHandle.IsValid())
			{
				TargetASC->RemoveActiveGameplayEffect(EscortedEffectHandle);
				EscortedEffectHandle.Invalidate();

				// 이송 중단(기절 등)인 경우 도둑을 Cuffed 상태로 전환
				if (bWasCancelled && IsValid(CuffedEffectClass))
				{
					FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(CuffedEffectClass, 1.f);
					TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}

			UThiefEscortComponent* EscortComp = TargetThief->GetThiefEscortComponent();
			if (IsValid(EscortComp))
			{
				EscortComp->SetEscortedBy(nullptr);
			}

			// 취소(발차기 등)인 경우에만 겹침 방지 밀어내기
			if (bWasCancelled)
			{
				AActor* PoliceActor = GetAvatarActorFromActorInfo();
				if (IsValid(PoliceActor))
				{
					TargetThief->AddActorWorldOffset(PoliceActor->GetActorRightVector() * 50.0f, true);
				}
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
};
