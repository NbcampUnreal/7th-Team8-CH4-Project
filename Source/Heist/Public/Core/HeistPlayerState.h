#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "HeistPlayerState.generated.h"

class UHeistAbilitySystemComponent;
class UHeistAttributeSet;

/**
 * ASC를 소유한다. 리스폰 후에도 GAS 상태가 유지된다.
 */
UCLASS()
class HEIST_API AHeistPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AHeistPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UHeistAbilitySystemComponent* GetHeistAbilitySystemComponent() const;

private:
	UPROPERTY()
	TObjectPtr<UHeistAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UHeistAttributeSet> AttributeSet;
};
