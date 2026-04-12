#include "Core/HeistPlayerState.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "AbilitySystem/HeistAttributeSet.h"

#include "Voice/HeistVoipTalker.h"

AHeistPlayerState::AHeistPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UHeistAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UHeistAttributeSet>(TEXT("AttributeSet"));

	VoipTalker = CreateDefaultSubobject<UHeistVoipTalker>(TEXT("VoipTalker"));

	// PlayerState는 NetUpdateFrequency 기본값이 낮으므로 GAS 반응성을 위해 높인다.
	SetNetUpdateFrequency(100.0f);
}

void AHeistPlayerState::BeginPlay()
{
	Super::BeginPlay();

	VoipTalker->RegisterWithPlayerState(this);
}

void AHeistPlayerState::OnSetUniqueId()
{
	Super::OnSetUniqueId();

	if (IsValid(VoipTalker))
	{
		VoipTalker->RegisterWithPlayerState(this);
	}
}

UAbilitySystemComponent* AHeistPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UHeistAbilitySystemComponent* AHeistPlayerState::GetHeistAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UHeistVoipTalker* AHeistPlayerState::GetVoipTalker() const
{
	return VoipTalker;
}
