#include "AbilitySystem/HeistAbilitySystemComponent.h"

#include "AbilitySystem/HeistGameplayAbility.h"
#include "AbilitySystemInterface.h"

UHeistAbilitySystemComponent* UHeistAbilitySystemComponent::FindAbilitySystemComponent(const AActor* Actor)
{
	if (!IsValid(Actor)) return nullptr;

	const IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Actor);
	if (ASCInterface != nullptr)
	{
		return Cast<UHeistAbilitySystemComponent>(ASCInterface->GetAbilitySystemComponent());
	}
	return nullptr;
}

void UHeistAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	InputPressedTags.AddUnique(InputTag);
}

void UHeistAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	InputReleasedTags.AddUnique(InputTag);
}

void UHeistAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	for (const FGameplayTag& Tag : InputPressedTags)
	{
		for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
		{
			if (!Spec.GetDynamicSpecSourceTags().HasTagExact(Tag)) continue;

			if (Spec.IsActive())
				AbilitySpecInputPressed(Spec);
			else
				TryActivateAbility(Spec.Handle);
		}
	}

	for (const FGameplayTag& Tag : InputReleasedTags)
	{
		for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
		{
			if (!Spec.GetDynamicSpecSourceTags().HasTagExact(Tag)) continue;
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

	InputPressedTags.Reset();
	InputReleasedTags.Reset();
}
