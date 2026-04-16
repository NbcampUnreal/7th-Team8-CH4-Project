#include "Character/PoliceCharacter.h"

#include "Components/FlashlightComponent.h"
#include "Components/ThiefEscortComponent.h"
#include "Components/SoundDetectionComponent.h"

#include "GameFramework/CharacterMovementComponent.h"

APoliceCharacter::APoliceCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCharacterMovement()->RotationRate = FRotator(0.f, MaxRotationRateYaw, 0.f);

	FlashlightComponent = CreateDefaultSubobject<UFlashlightComponent>(TEXT("FlashlightComponent"));
	EscortComponent = CreateDefaultSubobject<UThiefEscortComponent>(TEXT("EscortComponent"));
	SoundDetectionComponent = CreateDefaultSubobject<USoundDetectionComponent>(TEXT("SoundDetectionComponent"));
}

void APoliceCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (IsValid(FlashlightComponent))
	{
		FlashlightComponent->TryStartLocalVision();
	}
}

void APoliceCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (IsValid(FlashlightComponent))
	{
		FlashlightComponent->TryStartLocalVision();
	}
}

void APoliceCharacter::UnPossessed()
{
	if (IsValid(FlashlightComponent))
	{
		FlashlightComponent->StopLocalVision();
	}

	Super::UnPossessed();
}
