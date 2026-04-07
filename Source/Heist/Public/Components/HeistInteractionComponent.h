
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "HeistInteractionComponent.generated.h"

class IHeistInteractable;

UCLASS(ClassGroup=(Heist), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	// IHeistInteractable 오브젝트 Sphere 안에서 호출될 Overlap 이벤트 
	// TScriptInterface - Raw pointer로 된 Interface와 달리 BP에서 사용 가능함
	void RegisterInteractable(TScriptInterface<IHeistInteractable> Interactable);
	void UnregisterInteractable(TScriptInterface<IHeistInteractable> Interactable);
	
	// LMB 입력 시 호출됨 - Line Trace 히트 Actor 검증 후 어빌리티 Tag를 반환한다.
	// 목록에 없거나 CanInteract == false이면 EmptyTag를 반환하도록 한다.
	FGameplayTag ResolveInteractAbilityTag(AActor* HitActor);
	
private:
	// 현재 캐릭터 Sphere 안에 들어온 IHeistInteractable 목록을 가져온다.
	TArray<TScriptInterface<IHeistInteractable>> ActiveInteractables;
};
