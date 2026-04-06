#include "AbilitySystem/Police/GA_PoliceCuffing.h"

#include "Character/ThiefCharacter.h"
#include "Character/HeistTags_State.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "AbilitySystemComponent.h"

#include "Kismet/KismetSystemLibrary.h"

UGA_PoliceCuffing::UGA_PoliceCuffing()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_PoliceCuffing::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

	TargetThief = nullptr;

	TArray<AActor*> OverlappingActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetAvatarActorFromActorInfo());

	//TODO(하민): 임시 타겟팅 (추후 Interaction 컴포넌트로 교체)
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetAvatarActorFromActorInfo()->GetActorLocation(),
		InteractRadius, ObjectTypes, AThiefCharacter::StaticClass(), ActorsToIgnore, OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		AThiefCharacter* Thief = Cast<AThiefCharacter>(Actor);
		if (!IsValid(Thief)) continue;

		UAbilitySystemComponent* TargetASC = Thief->GetAbilitySystemComponent();
		if (IsValid(TargetASC) && TargetASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured))
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

	StartChanneling(FName("Cuffing"));
}

void UGA_PoliceCuffing::OnChannelingCompleted()
{
	if (IsValid(TargetThief))
	{
		UAbilitySystemComponent* TargetASC = TargetThief->GetAbilitySystemComponent();
		if (IsValid(TargetASC))
		{
			FGameplayEventData Payload;
			Payload.Instigator = GetAvatarActorFromActorInfo();
			Payload.Target = TargetThief;

			TargetASC->HandleGameplayEvent(HeistEventTags::Event_CuffingComplete, &Payload);

			// [테스트 코드] 도둑 파트가 없어서 경찰이 직접 태그 교체
			TargetASC->RemoveLooseGameplayTag(HeistStateTags::State_Thief_Injured);
			TargetASC->AddLooseGameplayTag(HeistStateTags::State_Thief_Cuffed);
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PoliceCuffing::OnChannelingCancelled()
{
	TargetThief = nullptr;
}
