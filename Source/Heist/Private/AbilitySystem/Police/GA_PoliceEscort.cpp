#include "AbilitySystem/Police/GA_PoliceEscort.h"

#include "Character/ThiefCharacter.h"
#include "Character/HeistTags_State.h"
#include "Components/ThiefEscortComponent.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "AbilitySystemComponent.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_PoliceEscort::UGA_PoliceEscort()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(HeistStateTags::State_Police_Escorting);
	ActivationBlockedTags.AddTag(HeistStateTags::State_Police_Escorting);
	CancelAbilitiesWithTag.AddTag(HeistStateTags::State_Stunned);
}

void UGA_PoliceEscort::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

	TargetThief = nullptr;

	TArray<AActor*> OverlappingActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes{ UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn) };
	TArray<AActor*> ActorsToIgnore{ GetAvatarActorFromActorInfo() };

	// TODO(하민): 임시 타겟팅 (추후 Interaction 컴포넌트로 교체)
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetAvatarActorFromActorInfo()->GetActorLocation(),
		150.0f, ObjectTypes, AThiefCharacter::StaticClass(), ActorsToIgnore, OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		AThiefCharacter* Thief = Cast<AThiefCharacter>(Actor);
		if (!IsValid(Thief)) continue;

		UAbilitySystemComponent* TargetASC = Thief->GetAbilitySystemComponent();
		if (IsValid(TargetASC) && TargetASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed))
		{
			TargetThief = Thief;
			break;
		}
	}

	if (!IsValid(TargetThief))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetThief->GetAbilitySystemComponent();

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
