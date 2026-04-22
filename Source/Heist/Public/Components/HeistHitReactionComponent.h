#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "HeistHitReactionComponent.generated.h"

/*
 * 근접 타격 처리용 non-dynamic delegate.
 * 공격 Ability가 현재 타격 처리 함수를 등록하고, HitReactionComponent가 공통 진입점에서 실행합니다.
 */
DECLARE_DELEGATE_OneParam(FHeistMeleeHitDelegate, const FGameplayEventData&);

/*
 * melee hit 공통 진입점 컴포넌트.
 * 현재 공격 Ability가 hit handler를 등록하면, AnimNotifyState는 이 컴포넌트를 통해 타격 판정 결과를 전달합니다.
 * 향후 피격 연출, 캐시, 면역 시간 등 공통 피격 인프라도 이 컴포넌트에서 확장할 수 있습니다.
	* 피격 연출의 공통 인프라
	- 마지막 공격자 저장
	- 마지막 피격 위치/방향 저장
	- 히트 스톱, 카메라 셰이크, 이펙트/사운드 브로드캐스트
	- 연속 피격 면역 시간 관리
	- 로컬 전용 리액션 재생
	- 공통 충돌/피격 캐시

 * IGameFrameworkInitStateInterface의 구현은 아래 단계를 보장한다고 합니다.
	PawnExtension ──┐
				  ├──► InitState_DataAvailable    (PawnData 세팅됨)
				  ├──► InitState_DataInitialized  (ASC 초기화됨)
				  └──► InitState_GameplayReady    ◄─── 이 시점에 ASC 보장됨
							  │
	HitReactionComponent ─────┘
 *  HeistPlayerComponent 가 BindInput()에서 State_MoveDisabled 를 등록하는 것과 완전히 동일한 이유이며, ASC가 준비된 시점을 PawnExtension의 상태 머신에서 보장받는 구조라 하니 사용해보겠습니다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HEIST_API UHeistHitReactionComponent : public UActorComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UHeistHitReactionComponent(const FObjectInitializer& ObjectInitializer);

	static UHeistHitReactionComponent* FindHitReactionComponent(const AActor* Actor);
	static const FName NAME_ActorFeatureName;

	void SetMeleeHitHandler(const FHeistMeleeHitDelegate& InHandler);
	void ResetMeleeHitHandler();
	bool HasMeleeHitHandler() const;
	void ProcessMeleeHit(AActor* InstigatorActor, AActor* TargetActor) const;

	// IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitReaction(bool bIsHitValid, AActor* InstigatorActor, const FVector& ImpactLocation);

private:
	FHeistMeleeHitDelegate MeleeHitHandler;
};
