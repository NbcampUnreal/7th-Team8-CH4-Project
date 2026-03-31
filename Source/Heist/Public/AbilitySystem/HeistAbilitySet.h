#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpecHandle.h"
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
 * GiveToAbilitySystem 호출 결과로 부여된 어빌리티 핸들 묶음.
 * TakeFromAbilitySystem을 호출하여 부여된 어빌리티를 모두 회수한다.
 */
USTRUCT(BlueprintType)
struct FHeistAbilitySetHandles
{
	GENERATED_BODY()

	void TakeFromAbilitySystem(UHeistAbilitySystemComponent* ASC);

	bool HasHandles() const { return AbilitySpecHandles.Num() > 0; }

private:
	friend class UHeistAbilitySet;

	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
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
	// OutHandles에 부여된 핸들을 채운다. OutHandles로 나중에 회수 가능.
	void GiveToAbilitySystem(UHeistAbilitySystemComponent* ASC, FHeistAbilitySetHandles* OutHandles = nullptr) const;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<FHeistAbilitySetEntry> GrantedAbilities;
};
