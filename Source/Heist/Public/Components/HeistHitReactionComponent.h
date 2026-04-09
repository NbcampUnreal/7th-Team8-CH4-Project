
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "HeistHitReactionComponent.generated.h"

/*
 * DECLARE_DELEGATE_OneParam - Melee Attack에 관한 처리를 위임합니다. ANS 등에서 실행된 GameplayEventData를 받아 실행 로직을 처리합니다.   
 */
DECLARE_DELEGATE_OneParam(FHeistMeleeHitDelegate, const FGameplayEventData&);

/*
 * 해당 컴포넌트는 향후, 피격 시의 로직(폭발, 섬광탄 등)까지 담당한다면 적합합니다, 현재는 Delegate 수행하기 위한 껍데기로 선언하겠습니다
 * IGameFrameworkInitStateInterface의 구현은 아래 단계를 보장한다고 합니다. 
	PawnExtension ──┐
				  ├──► InitState_DataAvailable    (PawnData 세팅됨)
				  ├──► InitState_DataInitialized  (ASC 초기화됨)
				  └──► InitState_GameplayReady    ◄─── 이 시점에 ASC 보장됨
							  │
	HitReactionComponent ─────┘
 *  HeistPlayerComponent 가 BindInput()에서 State_MoveDisabled 를 등록하는 것과 완전히 동일한 이유이며, ASC가 준비된 시점을 PawnExtension의 상태 머신에서 보장받는 구조라 하니 사용해보겠습니다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistHitReactionComponent : public UActorComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()
	
public:
	UHeistHitReactionComponent(const FObjectInitializer& ObjectInitializer);
	
	static UHeistHitReactionComponent* FindHitReactionComponent(const AActor* Actor);
	static const FName NAME_ActorFeatureName;
	
	// IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	FHeistMeleeHitDelegate OnMeleeHit;
	
private:
	// State_Stunned GE 만료 → Outro 섹션 점프
	void OnStunnedTagChanged(const FGameplayTag Tag, int32 Count);

};
