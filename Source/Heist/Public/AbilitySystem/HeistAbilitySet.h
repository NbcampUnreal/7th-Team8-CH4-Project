#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "HeistAbilitySet.generated.h"

class UHeistGameplayAbility;
class UHeistAbilitySystemComponent;

USTRUCT(BlueprintType)
struct FHeistAbilitySetEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UHeistGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly)
	int32 AbilityLevel = 1;

	// 입력 태그. 설정 시 이 태그로 어빌리티 활성화/종료를 라우팅한다.
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag InputTag;
};

/**
 * 캐릭터에 부여할 어빌리티 묶음.
 * PawnData가 참조하며, PawnExtensionComponent가 GameplayReady 시점에 ASC에 부여한다.
 */
UCLASS(BlueprintType, Const)
class HEIST_API UHeistAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	void GiveToAbilitySystem(UHeistAbilitySystemComponent* ASC) const;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<FHeistAbilitySetEntry> GrantedAbilities;
};
