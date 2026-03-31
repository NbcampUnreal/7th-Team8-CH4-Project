#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "HeistGameplayAbility.generated.h"

UENUM(BlueprintType)
enum class EHeistAbilityActivationPolicy : uint8
{
	OnInputTriggered,  // 입력 시 한 번 발동
	WhileInputActive,  // 입력 유지 중 활성
	OnSpawn,           // 부여 즉시 발동
};

/**
 * 모든 Heist GA의 베이스 클래스.
 *
 * - InputTag: ASC가 입력 이벤트를 이 어빌리티로 라우팅할 때 사용
 * - ActivationPolicy: 발동 조건 제어
 */
UCLASS(Abstract)
class HEIST_API UHeistGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	EHeistAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Ability")
	EHeistAbilityActivationPolicy ActivationPolicy = EHeistAbilityActivationPolicy::OnInputTriggered;
};
