#include "AbilitySystem/Common/GA_Sneak.h"

#include "Character/HeistTags_State.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Sneak::UGA_Sneak()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::WhileInputActive;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_Sneak::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(ASC)) return;

	ASC->AddReplicatedLooseGameplayTag(HeistStateTags::State_Sneaking);

	// TODO: AttributeSet 구현 후 GE로 교체
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (IsValid(Character))
	{
		OriginalMoveSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeed = SneakMoveSpeed;
	}
}

void UGA_Sneak::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (IsValid(ASC))
	{
		ASC->RemoveReplicatedLooseGameplayTag(HeistStateTags::State_Sneaking);
	}

	// TODO: AttributeSet 구현 후 GE로 교체
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (IsValid(Character))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = OriginalMoveSpeed;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
