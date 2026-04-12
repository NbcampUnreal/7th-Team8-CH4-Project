#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimInstance.h"
#include "HeistAnimInstance.generated.h"

class UAbilitySystemComponent;
class ACharacter;
class USkeletalMeshComponent;

UCLASS()
class HEIST_API UHeistAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
#pragma region AbilitySystem
	
public:
	void TryBindAbilitySystem();
	void UnbindAbilitySystem();
	void SetEscortPelvisPhysicsEnabled(bool bEnabled);
	
private:
	// StateTag 추가
	void SyncAllTagsEvents(UAbilitySystemComponent* ASC);
	void BindAllTagsEvents(UAbilitySystemComponent* ASC);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
	
public:
	// AnimGraph에서 사용할 상태 boolean
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsSneaking = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsStunned = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsKnockbacked = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsEscorted = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsCuffed = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsInjured = false;
	
private:
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	bool bTagBindingReady = false;
	
	FDelegateHandle SneakingTagChangedHandle;
	FDelegateHandle StunnedTagChangedHandle;
	FDelegateHandle KnockbackTagChangedHandle;
	FDelegateHandle CuffedTagChangedHandle;
	FDelegateHandle EscortedTagChangedHandle;
	FDelegateHandle InjuredTagChangedHandle;
	
#pragma endregion AbilitySystem
	
#pragma region AnimIK
private:
	void UpdateIK(float DeltaSeconds);
	void DoFootTrace(FName SocketName, FVector& OutOffset, FRotator& OutRotation, bool& OutHit, float DeltaSeconds);
	
	// 구속 상태(cuffed / escorted)에 공통으로 들어가는 연출 스위치를 계산한다.
	void ApplyRestraintPresentationState();
	void RefreshIKFootSpeedThresholdCached();

	void UpdateRestraintHandGoal();
	void ClearRestraintHandGoal();
	void ResetFootIKImmediate();
	
	USkeletalMeshComponent* ResolveRestraintGoalSource() const;
	
public:
	UPROPERTY(BlueprintReadOnly, Category="IK")
	FVector IK_Offset_Pelvis;

	UPROPERTY(BlueprintReadOnly, Category="IK")
	FVector IK_Offset_L;

	UPROPERTY(BlueprintReadOnly, Category="IK")
	FVector IK_Offset_R;

	UPROPERTY(BlueprintReadOnly, Category="IK")
	FRotator IK_Rotation_L;

	UPROPERTY(BlueprintReadOnly, Category="IK")
	FRotator IK_Rotation_R;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	float IK_Trace_Dist = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	float IK_InterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	float FootHeight = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	TEnumAsByte<ECollisionChannel> IK_TraceChannel = ECC_Visibility;

	UPROPERTY(BlueprintReadOnly, Category="IK")                                                                                                                                                                                                                       
	float IK_Alpha_L = 0.f;                                                                                                                                                                                                                                           
                                                                                                                                                                                                                                                                    
	UPROPERTY(BlueprintReadOnly, Category="IK")                                                                                                                                                                                                                     
	float IK_Alpha_R = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category="IK")
	float IK_ThighDeadZone = 27.5f;
	
	// 발이 빠르게 움직이면 IK 끄기 - 속도 임계점
	// UPROPERTY(EditAnywhere, Category="IK")
	// float IK_FootSpeedThreshold = 200.f;
	
	UPROPERTY(EditDefaultsOnly, Category="IK")
	float NormalThreshold = 0.5f;

	// 태그 별 임계값 테이블
	UPROPERTY(EditAnywhere, Category = "IK")
	TMap<FGameplayTag, float> IK_FootSpeedThresholdMap;
	
	// 기본 임계값 설정
	UPROPERTY(EditAnywhere, Category="IK")
	float IK_FootSpeedThreshold_Default = 200.f;
	
	// 수갑 또는 이송 상태를 한 번에 다루는 공통 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restraint")
	bool bIsRestrained = false;

	// 오른손 구속 IK 사용 여부. 실제 상태에 따라 런타임에서 계산된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restraint")
	bool bUseRestrainedHandIK = false;

	// IK Goal 유효 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restraint")
	bool bHasRightHandGoal = false;

	// Character Mesh Component Space 기준 Goal 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restraint")
	FVector RightHandGoal_CS = FVector::ZeroVector;

	// IK Rig Goal PositionSpace=Additive 용 보정량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restraint")
	FVector RightHandGoalOffset_CS = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restraint")
	FRotator RightHandGoalRot_CS = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restraint")
	float RightHandGoalAlpha = 0.f;
	
	// 구속 상태 동안 발 IK를 끄고 싶을 때 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Restraint")
	bool bDisableFootIKWhenRestrained = true;

	// 구속 goal 소켓을 붙여 둔 컴포넌트에서 매 프레임 목표를 자동으로 읽어온다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Restraint")
	bool bAutoResolveRightHandGoal = true;

	// Escort 물리 연출 중에는 오른손 IK를 꺼서 물리 포즈와 충돌하지 않게 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Restraint")
	bool bDisableRestrainedHandIKWhileEscorted = true;
	
	// Goal 소켓을 가진 컴포넌트에 붙일 tag.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Restraint")
	FName RestraintGoalSourceComponentTag = TEXT("CuffGoalSource");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Restraint")
	FName RightHandGoalSocketName = TEXT("RightHandSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	bool bUseEscortPelvisPhysics = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	FName EscortPhysicsRootBone = TEXT("hips");

	// 양팔은 physics simulate 상태를 유지하되 blend를 0으로 두어 포즈는 애니메이션을 우선시한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	FName EscortPhysicsLeftArmBone = TEXT("upperarm_l");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	FName EscortPhysicsRightArmBone = TEXT("upperarm_r");

	// 흉곽부터 상체 chain을 simulate 상태로 두되 포즈는 잠가 둔다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	FName EscortPhysicsChestBone = TEXT("chest");

	// 다리는 다시 물리 chain에 포함해 끌리는 실루엣을 유지한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	FName EscortPhysicsLeftUpperLegBone = TEXT("upperleg_l");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	FName EscortPhysicsRightUpperLegBone = TEXT("upperleg_r");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	FName EscortPhysicsLeftFootBone = TEXT("foot_l");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	FName EscortPhysicsRightFootBone = TEXT("foot_r");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	float EscortPhysicsBlendWeight = 1.0f;

	// hips 루트 실험 시 capsule/movement와 과하게 충돌하지 않도록 더 낮은 물리 비중을 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	float EscortHipsBlendWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	float EscortLegBlendWeight = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	float EscortFootBlendWeight = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escort")
	float EscortLockedLimbBlendWeight = 0.0f;
	
	UPROPERTY()
	ACharacter* PlayerChar;
	
	FCollisionQueryParams TraceParams;
	
	// 로컬 공간 기준 이전 프레임 발 소켓 위치
	FVector PrevLocalL = FVector::ZeroVector;
	FVector PrevLocalR = FVector::ZeroVector;
	
	// 캐릭터 초기화 시 속도 스파이크 방지
	bool bLocalFootCacheInitialized = false;

	// 태그 기반 임계값 캐시(태그 변경 시점에만 갱신)
	float IK_FootSpeedThreshold_Cached = 200.f;

	UPROPERTY()
	bool bIK_HitL = false;

	UPROPERTY()
	bool bIK_HitR = false;
	
	static const FName FootL;
	static const FName FootR;
#pragma endregion AnimIK
	
protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
