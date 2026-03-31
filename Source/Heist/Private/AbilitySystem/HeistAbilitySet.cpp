#include "AbilitySystem/HeistAbilitySet.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "AbilitySystem/HeistGameplayAbility.h"

void UHeistAbilitySet::GiveToAbilitySystem(UHeistAbilitySystemComponent* ASC) const
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
		ASC->GiveAbility(Spec);
	}
}
