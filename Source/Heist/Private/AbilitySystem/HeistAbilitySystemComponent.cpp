#include "AbilitySystem/HeistAbilitySystemComponent.h"

#include "AbilitySystem/HeistGameplayAbility.h"

void UHeistAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag)) continue;

		if (Spec.IsActive())
		{
			AbilitySpecInputPressed(Spec);
		}
		else
		{
			TryActivateAbility(Spec.Handle);
		}
	}
}

void UHeistAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag)) continue;

		if (!Spec.IsActive()) continue;

		AbilitySpecInputReleased(Spec);

		const UHeistGameplayAbility* HeistAbility = Cast<UHeistGameplayAbility>(Spec.Ability);
		if (IsValid(HeistAbility)
			&& HeistAbility->GetActivationPolicy() == EHeistAbilityActivationPolicy::WhileInputActive)
		{
			CancelAbilityHandle(Spec.Handle);
		}
	}
}
