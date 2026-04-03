#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataTable.h"
#include "HeistGameplayAbility.generated.h"

UENUM(BlueprintType)
enum class EHeistAbilityActivationPolicy : uint8
{
	OnInputTriggered,  // 입력 시 한 번 발동
	WhileInputActive,  // 입력 유지 중 활성
	OnSpawn,           // 부여 즉시 발동
};

/**
 * StartChanneling()에서 참조하는 DataTable 행 구조체.
 * DT_ChannelingData 애셋에 어빌리티별 채널링 설정을 정의한다.
 */
USTRUCT(BlueprintType)
struct FChannelingData : public FTableRowBase
{
	GENERATED_BODY()

	// 채널링 완료까지 걸리는 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Duration = 1.0f;

	// 채널링 중 이동 가능 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bCanMove = true;

	// 이동 시 자동 취소 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bCancelOnMove = true;

	// 피격 시 자동 취소 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bCancelOnHit = true;

	// 외부 중단 이벤트 Tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag InterruptEventTag;
};

/**
 * 모든 Heist GA의 베이스 클래스.
 *
 * ActivationPolicy: 발동 조건 제어
 * StartChanneling(): 채널링 어빌리티 공통 패턴
 *   → DataTable에서 Duration을 읽어 타이머 시작
 *   → 완료 시 OnChannelingCompleted(), 취소 시 OnChannelingCancelled() 호출
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

	// 채널링 어빌리티용 DataTable. RowName별로 Duration 등을 정의한다.
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Channeling")
	TObjectPtr<UDataTable> ChannelingDataTable;

	// RowName으로 DataTable에서 채널링 데이터를 조회한다.
	const FChannelingData* GetChannelingData(FName RowName) const;

	// 채널링을 시작한다. ActivateAbility에서 호출.
	void StartChanneling(FName RowName);

	// 채널링 완료 시 호출. 서브클래스에서 오버라이드하여 결과를 처리한다.
	virtual void OnChannelingCompleted() {}

	// 채널링 취소 시 호출. 서브클래스에서 오버라이드하여 정리를 처리한다.
	virtual void OnChannelingCancelled() {}

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnChannelingTimerExpired();

	UFUNCTION()
	void OnChannelingInterruptEvent(FGameplayEventData Payload);

	bool bIsChanneling = false;
};
