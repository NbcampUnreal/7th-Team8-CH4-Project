#include "Core/HeistPlayerState.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"

AHeistPlayerState::AHeistPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UHeistAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// PlayerState는 NetUpdateFrequency 기본값이 낮으므로 GAS 반응성을 위해 높인다.
	NetUpdateFrequency = 100.0f;
}

UAbilitySystemComponent* AHeistPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UHeistAbilitySystemComponent* AHeistPlayerState::GetHeistAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
