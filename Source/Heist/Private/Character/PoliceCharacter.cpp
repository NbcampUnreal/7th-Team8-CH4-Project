#include "Character/PoliceCharacter.h"

#include "Character/HeistTags_State.h"
#include "Components/FlashlightComponent.h"
#include "Components/ThiefEscortComponent.h"
#include "Components/SoundDetectionComponent.h"
#include "Components/HeistZoneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

APoliceCharacter::APoliceCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCharacterMovement()->RotationRate = FRotator(0.f, MaxRotationRateYaw, 0.f);

	FlashlightComponent = CreateDefaultSubobject<UFlashlightComponent>(TEXT("FlashlightComponent"));
	EscortComponent = CreateDefaultSubobject<UThiefEscortComponent>(TEXT("EscortComponent"));
	SoundDetectionComponent = CreateDefaultSubobject<USoundDetectionComponent>(TEXT("SoundDetectionComponent"));
	ZoneComponent = CreateDefaultSubobject<UHeistZoneComponent>(TEXT("ZoneComponent"));

	// 1. 손전등 (SpotLight) 설정
	FlashlightSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightSpotLight"));
	FlashlightSpotLight->SetupAttachment(RootComponent);
	FlashlightSpotLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.85f));
	FlashlightSpotLight->Intensity = FlashlightIntensity;
	FlashlightSpotLight->AttenuationRadius = FlashlightRadius;
	FlashlightSpotLight->InnerConeAngle = FlashlightInnerConeAngle;
	FlashlightSpotLight->OuterConeAngle = FlashlightOuterConeAngle;
	FlashlightSpotLight->CastShadows = true;

	// 2. 근접 시야 (PointLight) 설정
	CloseVisionPointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CloseVisionPointLight"));
	CloseVisionPointLight->SetupAttachment(RootComponent);
	CloseVisionPointLight->SetLightColor(FLinearColor(0.5f, 0.5f, 0.5f));
	CloseVisionPointLight->Intensity = CloseVisionIntensity;
	CloseVisionPointLight->AttenuationRadius = CloseVisionRadius;
	CloseVisionPointLight->CastShadows = false;

	CloseVisionPointLight->SetVisibility(false);
}

void APoliceCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (IsLocallyControlled())
	{
		if (IsValid(FlashlightComponent))
		{
			FlashlightComponent->TryStartLocalVision();
		}

		TryBindZoneTagListener();
	}
}

void APoliceCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (IsLocallyControlled())
	{
		if (IsValid(FlashlightComponent))
		{
			FlashlightComponent->TryStartLocalVision();
		}

		TryBindZoneTagListener();
	}
}

void APoliceCharacter::UnPossessed()
{
	if (IsValid(FlashlightComponent))
	{
		FlashlightComponent->StopLocalVision();
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (IsValid(ASC) && ZoneTagListenerHandle.IsValid())
	{
		ASC->RegisterGameplayTagEvent(HeistStateTags::Zone_Indoor, EGameplayTagEventType::NewOrRemoved).Remove(ZoneTagListenerHandle);
		ZoneTagListenerHandle.Reset();
	}

	Super::UnPossessed();
}

void APoliceCharacter::TryBindZoneTagListener()
{
	GetWorld()->GetTimerManager().ClearTimer(ASCBindTimerHandle);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || ASC->GetAvatarActor() != this)
	{
		UWorld* World = GetWorld();
		if (IsValid(World))
		{
			constexpr float RetryDelay = 0.1f;
			World->GetTimerManager().SetTimer(ASCBindTimerHandle, this, &APoliceCharacter::TryBindZoneTagListener, RetryDelay, false);
		}
		return;
	}

	if (ZoneTagListenerHandle.IsValid())
	{
		ASC->RegisterGameplayTagEvent(HeistStateTags::Zone_Indoor, EGameplayTagEventType::NewOrRemoved).Remove(ZoneTagListenerHandle);
	}

	ZoneTagListenerHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::Zone_Indoor,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &APoliceCharacter::OnZoneTagChanged);

	if (IsValid(CloseVisionPointLight))
	{
		const bool bIsIndoor = ASC->HasMatchingGameplayTag(HeistStateTags::Zone_Indoor);
		CloseVisionPointLight->SetVisibility(bIsIndoor);
	}
}

void APoliceCharacter::OnZoneTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (!IsValid(CloseVisionPointLight)) return;

	const bool bIsIndoor = (NewCount > 0);
	CloseVisionPointLight->SetVisibility(bIsIndoor);

}
