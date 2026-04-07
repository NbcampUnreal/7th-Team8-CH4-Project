
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

FGameplayTag UHeistInteractionComponent::ResolveInteractAbilityTag(AActor* HitActor)
{
	if (!IsValid(HitActor)) return FGameplayTag::EmptyTag;
	
	// (Destroy로 Unregister 누락된 경우) 쓰레기 강제 정리
	ActiveInteractables.RemoveAll([](const TScriptInterface<IHeistInteractable>& E)
	{
		return !IsValid(E.GetObject());
	});
	
	ACharacter* Interactor = Cast<ACharacter>(GetOwner());
	
	for (const TScriptInterface<IHeistInteractable>& Entity : ActiveInteractables)
	{
		// Hit Actor가 인터렉터블 하거나, 혹은 ActorComponent가 인터렉터블 하거나,
		UObject* EntityObj = Entity.GetObject();
		bool bOwnerMatch = (EntityObj == HitActor);
		
		// Hit Actor가 안 인터렉터블함
		if (!bOwnerMatch)
			if (UActorComponent* Comp = Cast<UActorComponent>(EntityObj)) // 그럼 혹시 Interactable Component?
				bOwnerMatch = (Comp->GetOwner() == HitActor);
		
		if (!bOwnerMatch) continue; // 아니면 폐기
		
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
