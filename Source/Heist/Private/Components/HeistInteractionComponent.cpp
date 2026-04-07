
#include "Components/HeistInteractionComponent.h"

#include "GameFramework/Character.h"
#include "Interaction/HeistInteractable.h"

void UHeistInteractionComponent::RegisterInteractable(TScriptInterface<IHeistInteractable> Interactable)
{
	if (Interactable.GetObject())
	{
		ActiveInteractables.AddUnique(Interactable);
	}
}

void UHeistInteractionComponent::UnregisterInteractable(TScriptInterface<IHeistInteractable> Interactable)
{
	ActiveInteractables.Remove(Interactable);
}

FGameplayTag UHeistInteractionComponent::ResolveInteractAbilityTag(AActor* HitActor) const
{
	if (!IsValid(HitActor)) return FGameplayTag::EmptyTag;
	
	ACharacter* Interactor = Cast<ACharacter>(GetOwner());
	
	for (const TScriptInterface<IHeistInteractable>& Entity : ActiveInteractables)
	{
		if (Entity.GetObject() != HitActor) continue; // Hit Actor가 아니면 건너뛰고
		
		// 거리 안에 엔티티 있으면 상호작용 가능한지 체크한다
		if (IHeistInteractable::Execute_CanInteract(Entity.GetObject(), Interactor))
		{
			return IHeistInteractable::Execute_GetInteractAbilityTag(Entity.GetObject(), Interactor);
		}
		
		// 목록에 있지만 상호작용 필요조건이 불충족됨 (CanInteract == false)
		return FGameplayTag::EmptyTag;
	}
	
	// 주변에 암것도 없다
	return FGameplayTag::EmptyTag;
}
