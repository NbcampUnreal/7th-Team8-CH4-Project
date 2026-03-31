#include "AbilitySystem/HeistAttributeSet.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UHeistAttributeSet::UHeistAttributeSet()
{
	// DataTable이 설정되지 않은 경우를 위한 폴백 기본값.
	// PawnData->CharacterStatsTable이 설정되면 DataInitialized 시점에 덮어쓴다.
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitMoveSpeed(400.0f);
}

void UHeistAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UHeistAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHeistAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHeistAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void UHeistAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (!IsValid(ASC)) return;

		ACharacter* Character = Cast<ACharacter>(ASC->GetAvatarActor());
		if (IsValid(Character))
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(NewValue, 0.0f);
		}
	}
}

void UHeistAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHeistAttributeSet, Health, OldHealth);
}

void UHeistAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHeistAttributeSet, MaxHealth, OldMaxHealth);
}

void UHeistAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHeistAttributeSet, MoveSpeed, OldMoveSpeed);
}
