#include "HeistThiefSlotWidget.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"

void UHeistThiefSlotWidget::BindToASC(UHeistAbilitySystemComponent* InASC)
{
	if (!InASC) return;
	CachedASC = InASC;

	CachedASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UHeistThiefSlotWidget::OnGameplayEffectAdded);
}

void UHeistThiefSlotWidget::OnGameplayEffectAdded(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle AGEHandle)
{
	FString StateName = Spec.Def->GetName();
	OnStatusChanged(StateName);
}
