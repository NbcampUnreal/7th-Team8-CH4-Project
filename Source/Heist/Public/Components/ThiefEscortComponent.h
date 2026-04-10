#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Character/HeistCharacter.h"
#include "Components/ActorComponent.h"
#include "ThiefEscortComponent.generated.h"

class UGameplayEffect;
class AThiefCharacter;
class AHeistCharacter;

/*
 * UThiefEscortComponent 를 escort 관계의 단일 관리자이다. 도둑과 경찰은 서로 체포관계에 있을 경우
 * 이 컴포넌트를 참조하여 그 종속을 빠르게 파악할 수 있다. 부가적인 동작 함수도 포함한다.
 * 
 */
UCLASS(ClassGroup = (Heist), meta = (BlueprintSpawnableComponent))
class HEIST_API UThiefEscortComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UThiefEscortComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Escort 관계 시작
	bool BeginEscort(
		AHeistCharacter* InPolice,
		TSubclassOf<UGameplayEffect> EscortedEffectClass,
		UAbilitySystemComponent* SourceASC);
	
	// Escort 관계 종료
	// bConvertToCuffed == true  : 기존 체포 상태 유지용 취소 - Default(기획)
	// bConvertToCuffed == false : Kick 등으로 탈출
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
	
	// 경찰이 자신이 escort 중인 도둑을 바로 찾을 때 사용하는 Static 함수
	static AThiefCharacter* FindEscortedThiefByPolice(const AHeistCharacter* InPolice);
	
private:
	UPROPERTY(EditDefaultsOnly, Category="Heist|Escort")
	TSubclassOf<UGameplayEffect> DefaultCuffedEffectClass;
	
	FActiveGameplayEffectHandle EscortedEffectHandle;
	
	UPROPERTY(ReplicatedUsing = OnRep_EscortedBy)
	TObjectPtr<AHeistCharacter> EscortedBy;
	
	void SetEscortedBy(AHeistCharacter* InPolice);
	
	UFUNCTION()
	void OnRep_EscortedBy();

	// 경찰 후방 동기화 거리
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float EscortOffsetDistance = 80.0f;
};
