#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "AbilitySystem/HeistAbilitySet.h"
#include "HeistPawnExtensionComponent.generated.h"

class UHeistPawnData;
class UHeistAbilitySystemComponent;
class UGameFrameworkComponentManager;
struct FActorInitStateChangedParams;

/**
 * GAS 초기화와 InitState 머신을 담당하는 컴포넌트.
 * IGameFrameworkInitStateInterface를 구현하여 UE 표준 InitState 흐름을 사용한다.
 *
 * InitState 흐름:
 *   InitState_DataAvailable   - PawnData가 설정됨
 *   InitState_DataInitialized - ASC가 초기화됨
 *   InitState_GameplayReady   - 모든 준비 완료, 입력 바인딩 가능
 */
UCLASS()
class HEIST_API UHeistPawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UHeistPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);

	static UHeistPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor);

	static const FName NAME_ActorFeatureName;

	// IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetPawnData(const UHeistPawnData* InPawnData);
	const UHeistPawnData* GetPawnData() const { return PawnData; }

	void InitializeAbilitySystem(UHeistAbilitySystemComponent* InASC, AActor* InOwnerActor);
	void UninitializeAbilitySystem();
	UHeistAbilitySystemComponent* GetAbilitySystemComponent() const { return AbilitySystemComponent; }

private:
	void ApplyCharacterStats();

	TObjectPtr<const UHeistPawnData> PawnData;

	UPROPERTY()
	TObjectPtr<UHeistAbilitySystemComponent> AbilitySystemComponent;

	FHeistAbilitySetHandles GrantedAbilitySetHandles;
};
