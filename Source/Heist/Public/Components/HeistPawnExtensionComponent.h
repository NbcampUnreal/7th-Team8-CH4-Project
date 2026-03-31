#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "HeistPawnExtensionComponent.generated.h"

class UHeistPawnData;
class UHeistAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE(FOnHeistGameplayReady);

/**
 * GAS 초기화와 InitState 머신을 담당하는 컴포넌트.
 * AHeistCharacter가 소유하며, 다른 컴포넌트들은 이 컴포넌트를 통해
 * 초기화 완료 시점을 구독한다.
 *
 * InitState 흐름:
 *   InitState_DataAvailable   - PawnData가 설정됨
 *   InitState_DataInitialized - ASC가 초기화됨
 *   InitState_GameplayReady   - 모든 준비 완료, 입력 바인딩 가능
 */
UCLASS()
class HEIST_API UHeistPawnExtensionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeistPawnExtensionComponent();

	static UHeistPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetPawnData(const UHeistPawnData* InPawnData);
	const UHeistPawnData* GetPawnData() const { return PawnData; }

	void InitializeAbilitySystem(UHeistAbilitySystemComponent* InASC, AActor* InOwnerActor);
	void UninitializeAbilitySystem();
	UHeistAbilitySystemComponent* GetAbilitySystemComponent() const { return AbilitySystemComponent; }

	// 현재 State가 주어진 State 이상에 도달했는지 확인한다.
	bool HasReachedInitState(const FGameplayTag& State) const;

	// 전환 가능한 State까지 순서대로 진행을 시도한다.
	void CheckDefaultInitialization();

	// GameplayReady 도달 시 브로드캐스트
	FOnHeistGameplayReady OnGameplayReady;

private:
	bool CanTransitionToState(const FGameplayTag& NewState) const;
	void HandleInitStateTransition(const FGameplayTag& NewState);

	TObjectPtr<const UHeistPawnData> PawnData;

	UPROPERTY()
	TObjectPtr<UHeistAbilitySystemComponent> AbilitySystemComponent;

	FGameplayTag CurrentInitState;
};
