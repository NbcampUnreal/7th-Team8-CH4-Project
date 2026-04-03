#include "Character/ThiefCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AThiefCharacter::AThiefCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCharacterMovement()->RotationRate = FRotator(0.f, MaxRotationRateYaw, 0.f);
}
