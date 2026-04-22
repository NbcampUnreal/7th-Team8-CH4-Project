#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/HeistGameplayAbility.h"
#include "GA_PoliceEscort.generated.h"

class AThiefCharacter;
class UAudioComponent;

UCLASS()
class HEIST_API UGA_PoliceEscort : public UHeistGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PoliceEscort();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnArrivedAtCar(FGameplayEventData Payload);

private:
	// 체포 성공시 경찰에게 부착할 GE
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	TSubclassOf<UGameplayEffect> EscortingEffectClass;

	// 체포시 도둑에게 부착할 GE
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	TSubclassOf<UGameplayEffect> EscortedEffectClass;

	// 이송 중단 시 도둑에게 부착할 GE (Escorted -> Cuffed 전환)
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Effects")
	TSubclassOf<UGameplayEffect> CuffedEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Heist|Escort")
	float EscortReleaseOffset = 50.0f;

	//FGameplayEffectSpecHandle EscortingEffectHandle;
	// Spec은 설계도이다. GE 적용 전, 파라미터를 통해 설정할 때 들고있는 용도이다.
	// 찍어낼 템플릿으로서 가지고 있다가 해당하는 GE를 만들어 실행하는 용도

	//Escort의 경우 ASC 적용 이후 핸들인 FActiveGameplayEffectHandle를 캐싱해야 한다.
	//가지고 있는 GE를 꼬집어 제거할 때 필요함
	FActiveGameplayEffectHandle EscortingEffectHandle;
	FActiveGameplayEffectHandle EscortedEffectHandle;

	UPROPERTY()
	TObjectPtr<AThiefCharacter> TargetThief;

	UPROPERTY()
	TObjectPtr<UAudioComponent> EscortAudioComp;
};
