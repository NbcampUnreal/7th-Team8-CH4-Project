#include "AbilitySystem/HeistAbilitySet.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "AbilitySystemComponent.h"

void FHeistAbilitySetHandles::TakeFromAbilitySystem(UHeistAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC)) return;

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	AbilitySpecHandles.Reset();
}

void UHeistAbilitySet::GiveToAbilitySystem(UHeistAbilitySystemComponent* ASC, FHeistAbilitySetHandles* OutHandles) const
{
	if (!IsValid(ASC)) return;

	for (const FHeistAbilitySetEntry& Entry : GrantedAbilities)
	{
		if (!IsValid(Entry.AbilityClass)) continue;

		FGameplayAbilitySpec Spec(Entry.AbilityClass, Entry.AbilityLevel);
		if (Entry.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(Entry.InputTag);
		}

		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);

		if (OutHandles != nullptr)
		{
			OutHandles->AbilitySpecHandles.Add(Handle);
		}

		const UHeistGameplayAbility* AbilityCDO = Entry.AbilityClass.GetDefaultObject();
		if (IsValid(AbilityCDO) && AbilityCDO->GetActivationPolicy() == EHeistAbilityActivationPolicy::OnSpawn)
		{
			ASC->TryActivateAbility(Handle);
		}
	}
}
