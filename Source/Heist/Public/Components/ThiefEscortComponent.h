
#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Character/HeistCharacter.h"
#include "Components/ActorComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "ThiefEscortComponent.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class UGameFrameworkComponentManager;
class UHeistAbilitySystemComponent;
class UHeistAnimInstance;
class USkeletalMeshComponent;
class USceneComponent;
class UCableComponent;
class AThiefCharacter;
class AHeistCharacter;
struct FActorInitStateChangedParams;

/*
 * UThiefEscortComponent는 escort 관계의 단일 관리자이다. 도둑과 경찰은 서로 체포관계에 있을 경우
 * 이 컴포넌트를 참조하여 그 종속을 빠르게 파악할 수 있다. 부가적인 동작 함수도 포함한다.
 * 
 */
UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UThiefEscortComponent : public UActorComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UThiefEscortComponent();

	static const FName NAME_ActorFeatureName;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;

	// Escort 관계 시작
	bool BeginEscort(
		AHeistCharacter* InPolice,
		TSubclassOf<UGameplayEffect> EscortedEffectClass,
		UAbilitySystemComponent* SourceASC);
	
	// Escort 관계 종료
	// bConvertToCuffed == true  : 기존 체포 상태 유지용 취소 - Default(기획)
	// bConvertToCuffed == false : 수갑 해제가 필요한 경우 사용 하세요
	void InterruptEscort(
		UAbilitySystemComponent* SourceASC,
		bool bConvertToCuffed = true);
	
	// 호환 오버로드
	void InterruptEscort(
		TSubclassOf<UGameplayEffect> InCuffedEffectClass,
		UAbilitySystemComponent* SourceASC,
		bool bConvertToCuffed = true);
	
	// 현재 이송 중 인지 체크
	UFUNCTION(BlueprintPure, Category = "Heist|Escort")
	bool IsEscorted() const { return IsValid(EscortedBy); }
	
	// 이송 by InCops
	bool IsEscortedBy(const AHeistCharacter* InPolice) const;
	
	UFUNCTION(BlueprintCallable, Category = "Heist|Escort")
	AHeistCharacter* GetEscortedBy() const { return EscortedBy; }

	UFUNCTION(BlueprintCallable, Category = "Heist|Escort")
	AThiefCharacter* GetEscortingThief() const { return EscortingThief; }
	
	// 경찰이 자신이 escort 중인 도둑을 바로 찾을 때 사용하는 Static 함수
	static AThiefCharacter* FindEscortedThiefByPolice(const AHeistCharacter* InPolice);

	template <typename TComponent>
	static TComponent* FindNamedComponent(const AActor* Actor, const FName ComponentName)
	{
		if (!IsValid(Actor) || ComponentName.IsNone())
		{
			return nullptr;
		}

		TArray<TComponent*> Components;
		Actor->GetComponents<TComponent>(Components);

		for (TComponent* Component : Components)
		{
			if (IsValid(Component) && Component->GetFName() == ComponentName)
			{
				return Component;
			}
		}

		return nullptr;
	}
	
private:
	UPROPERTY(EditDefaultsOnly, Category="Heist|Escort")
	TSubclassOf<UGameplayEffect> DefaultCuffedEffectClass;
	
	FActiveGameplayEffectHandle EscortedEffectHandle;
	
	UPROPERTY(ReplicatedUsing = OnRep_EscortedBy)
	TObjectPtr<AHeistCharacter> EscortedBy;

	UPROPERTY(ReplicatedUsing = OnRep_EscortingThief)
	TObjectPtr<AThiefCharacter> EscortingThief;

	void SetEscortedBy(AHeistCharacter* InPolice);
	void SetEscortingThief(AThiefCharacter* InThief);
	void ApplyEscortReplicationState();
	void BindVisualState(UHeistAbilitySystemComponent* ASC);
	void UnbindVisualState();
	void CacheVisualComponents();
	void CacheCuffComponent();
	void CacheRopeFromPolice(AHeistCharacter* InPolice);
	void UpdateVisualState();
	void SetComponentVisualHidden(USceneComponent* Component, bool bHidden) const;
	void HandleVisualTagChanged(const FGameplayTag Tag, int32 NewCount);
	
	UFUNCTION()
	void OnRep_EscortedBy();

	UFUNCTION()
	void OnRep_EscortingThief();

	// 경찰 후방 동기화 거리
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float EscortOffsetDistance = 140.0f;
	
	// 목표점으로 즉시 스냅하지 않고 늦게 따라가도록 보간 속도를 둔다.
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float EscortFollowInterpSpeed = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float EscortRotationInterpSpeed = 10.0f;

	// CharacterMovement 기반 이동 시 최대 속도 상한
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float EscortMaxSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float MeshFallThreshold = 500.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float TrailRecordInterval = 20.0f; // 경로 역추적 Interval

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float MeshVisualRecoveryDelay = 0.3f;
	
	// BP에서 부착한 경찰 cable component 이름. 없으면 시각 로직만 조용히 생략한다.
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort|Visual")
	FName RopeComponentName = TEXT("Rope");

	// BP에서 부착한 도둑 cuff mesh component 이름. 없으면 시각 로직만 조용히 생략한다.
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort|Visual")
	FName CuffComponentName = TEXT("Cuff");

	// 경찰 rope의 끝을 연결할 도둑 mesh 소켓 이름.
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort|Visual")
	FName RopeEndSocketName = TEXT("foot_l");
	
	// 경찰이 이동한 경로를 추적해야 하므로 배열이 필요함
	TArray<FVector> PolicePositionTrail;
	
	int32 MaxPositionTrailSize = 25;
	// 줄 끌어가기 거리 interval
	float LaunchIntervalDistance = 5.0f;
	
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> CachedCuffComponent;

	UPROPERTY()
	TObjectPtr<UCableComponent> CachedRopeComponent;

	UPROPERTY()
	TObjectPtr<UHeistAbilitySystemComponent> CachedASC;

	FDelegateHandle CuffedTagChangedHandle;
	FDelegateHandle EscortedTagChangedHandle;
	float MeshVisualRecoveryTimer = 0.0f;
	bool bVisualStateReady = false;
	bool bCachedCuffed = false;
	bool bCachedEscorted = false;
};
