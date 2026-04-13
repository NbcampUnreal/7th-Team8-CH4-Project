#include "Character/PoliceCharacter.h"

#include "Components/FlashlightComponent.h"

#include "GameFramework/CharacterMovementComponent.h"

APoliceCharacter::APoliceCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCharacterMovement()->RotationRate = FRotator(0.f, MaxRotationRateYaw, 0.f);

	FlashlightComponent = CreateDefaultSubobject<UFlashlightComponent>(TEXT("FlashlightComponent"));
}
