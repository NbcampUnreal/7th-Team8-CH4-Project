#include "Character/PoliceCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

APoliceCharacter::APoliceCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCharacterMovement()->RotationRate = FRotator(0.f, MaxRotationRateYaw, 0.f);
}
