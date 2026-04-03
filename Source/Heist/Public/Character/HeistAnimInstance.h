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
	void DoFootTrace(FName SocketName, FVector& OutOffset, FRotator& OutRotation, float DeltaSeconds);
	
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
	float IK_Trace_Dist = 50.f;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	float IK_InterpSpeed = 15.f;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	float FootHeight = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	TEnumAsByte<ECollisionChannel> IK_TraceChannel = ECC_Visibility;

	UPROPERTY(BlueprintReadOnly, Category="IK")                                                                                                                                                                                                                       
	float IK_Alpha_L = 0.f;                                                                                                                                                                                                                                           
                                                                                                                                                                                                                                                                    
	UPROPERTY(BlueprintReadOnly, Category="IK")                                                                                                                                                                                                                     
	float IK_Alpha_R = 0.f;

	UPROPERTY(EditDefaultsOnly, Category="IK")
	float IK_AlphaBlendThreshold = 8.f; // 이 이상 오프셋일 때 Alpha=1

private:
	UPROPERTY()
	ACharacter* PlayerChar;
	
	FCollisionQueryParams TraceParams;
	
	static const FName FootL;
	static const FName FootR;
#pragma endregion AnimIK
	
protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
