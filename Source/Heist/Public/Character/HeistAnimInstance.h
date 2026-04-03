// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HeistAnimInstance.generated.h"

struct FGameplayTag;
class UAbilitySystemComponent;
class ACharacter;
/**
 * 
 */
UCLASS()
class HEIST_API UHeistAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
#pragma region AbilitySystem
	
public:
	void TryBindAbilitySystem();
	void UnbindAbilitySystem();
	
private:
	// StateTag 추가
	void SyncAllTagsEvents(UAbilitySystemComponent* ASC);
	void BindAllTagsEvents(UAbilitySystemComponent* ASC);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
	
public:
	// AnimGraph에서 사용할 상태 boolean
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsSneaking = false;
	
private:
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	bool bTagBindingReady = false;
	
	FDelegateHandle SneakingTagChangedHandle;
#pragma endregion AbilitySystem
	
#pragma region AnimIK
private:
	void UpdateIK(float DeltaSeconds);
	void DoFootTrace(FName SocketName, FVector& OutOffset, FRotator& OutRotation, bool& OutHit, float DeltaSeconds);
	
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
	float IK_InterpSpeed = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	float FootHeight = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	TEnumAsByte<ECollisionChannel> IK_TraceChannel = ECC_Visibility;

	UPROPERTY(BlueprintReadOnly, Category="IK")                                                                                                                                                                                                                       
	float IK_Alpha_L = 0.f;                                                                                                                                                                                                                                           
                                                                                                                                                                                                                                                                    
	UPROPERTY(BlueprintReadOnly, Category="IK")                                                                                                                                                                                                                     
	float IK_Alpha_R = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category="IK")
	float IK_ThighDeadZone = 15.f;
	
	// 발이 빠르게 움직이면 IK 끄기 - 속도 임계점
	UPROPERTY(EditAnywhere, Category="IK")
	float IK_FootSpeedThreshold = 200.f;
	
	UPROPERTY(EditDefaultsOnly, Category="IK")
	float NormalThreshold = 0.5f;
	
private:
	UPROPERTY()
	ACharacter* PlayerChar;
	
	FCollisionQueryParams TraceParams;
	
	FVector PrevSocketL = FVector::ZeroVector;
	FVector PrevSocketR = FVector::ZeroVector;
	
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
