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
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(HeistStateTags::State_Police_Escorting);
	ActivationBlockedTags.AddTag(HeistStateTags::State_Police_Escorting);
	CancelAbilitiesWithTag.AddTag(HeistStateTags::State_Stunned);

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

	// 1. 도둑의 이동 차단
	UCharacterMovementComponent* ThiefMovement = TargetThief->GetCharacterMovement();
	if (IsValid(ThiefMovement))
	{
		ThiefMovement->SetMovementMode(MOVE_None);
	}

	// 2. 위치 추종 시작
	UThiefEscortComponent* EscortComp = TargetThief->GetThiefEscortComponent();
	if (IsValid(EscortComp))
	{
		EscortComp->SetEscortedBy(Cast<AHeistCharacter>(GetAvatarActorFromActorInfo()));
	}

	//TODO(하민): 경찰에게 이속 40% 감소 GE 적용

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
		// 1. 도둑 위치 추종 해제
		UThiefEscortComponent* EscortComp = TargetThief->GetThiefEscortComponent();
		if (IsValid(EscortComp))
		{
			EscortComp->SetEscortedBy(nullptr);
		}

		// 2. 도둑 이동 모드 걷기로 복구
		UCharacterMovementComponent* ThiefMovement = TargetThief->GetCharacterMovement();
		if (IsValid(ThiefMovement))
		{
			ThiefMovement->SetMovementMode(MOVE_Walking);
		}

		// 3. 취소(발차기 등)인 경우에만 겹침 방지 밀어내기
		if (bWasCancelled)
		{
			AActor* PoliceActor = GetAvatarActorFromActorInfo();
			if (IsValid(PoliceActor))
			{
				TargetThief->AddActorWorldOffset(PoliceActor->GetActorRightVector() * 50.0f, true);
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
};
