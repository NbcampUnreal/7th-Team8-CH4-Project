#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "HeistInteractable.generated.h"

// DT_InteractData 행 구조체
USTRUCT(BlueprintType)
struct FHeistInteractData : public FTableRowBase
{
	GENERATED_BODY()
	
	// InteractSphere 반경 (cm) - BeginPlay 시 적용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float InteractRadius = 100.f;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UHeistInteractable : public UInterface 
{
	GENERATED_BODY()
};

/**
 * 인터렉티브 수행하기 위해서 캐릭터가 상호작용 가능한 액터는 이 인터페이스를 구현해야 한다. 
 */
class HEIST_API IHeistInteractable
{
	GENERATED_BODY()

/* 아래 코드들, 순수 가상함수 안하고 BP 확장 고려해서 그냥 Implementation 패턴으로 통일합니다. 나중에 경찰차 문이나
 * 차량 출발시키기 등 CPP 구현이 거창한 것들은 그냥 BP로 뺄 수 있도록 하겠습니다.
 */
public:
	// 상호작용 가능 여부를 설정한다 - Interactor 상태(도둑인지 경찰인지)에 따라 동적으로 판단한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Heist|Interaction")
	bool CanInteract(ACharacter* Interactor) const;

	// 발동할 어빌리티 Tag 반환
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Heist|Interaction")
	FGameplayTag GetInteractAbilityTag(ACharacter* Interactor) const;

	// InteractSphere 반경 - DT_InteractData에서 읽음
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Heist|Interaction")
	float GetInteractRadius() const;
};
