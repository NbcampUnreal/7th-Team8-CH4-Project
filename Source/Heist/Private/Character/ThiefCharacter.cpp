#include "Character/ThiefCharacter.h"

#include "AbilitySystem/HeistTags_Ability.h"
#include "Character/HeistTags_State.h"
#include "Components/HeistInteractSphereComponent.h"
#include "Components/HeistTransparencyComponent.h"
#include "Components/ThiefEscortComponent.h"
#include "Components/HeistZoneComponent.h"
#include "Components/HeistNoiseComponent.h"
#include "Components/FlashlightDetectionComponent.h"
#include "Data/HeistSoundData.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PointLightComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AThiefCharacter::AThiefCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCharacterMovement()->RotationRate = FRotator(0.f, MaxRotationRateYaw, 0.f);

	InteractSphereComp = CreateDefaultSubobject<UHeistInteractSphereComponent>(TEXT("InteractSphereComp"));
	TransparencyComponent = CreateDefaultSubobject<UHeistTransparencyComponent>(TEXT("TransparencyComponent"));
	EscortComponent = CreateDefaultSubobject<UThiefEscortComponent>(TEXT("EscortComponent"));
	NoiseComponent = CreateDefaultSubobject<UHeistNoiseComponent>(TEXT("NoiseComponent"));
	ZoneComponent = CreateDefaultSubobject<UHeistZoneComponent>(TEXT("ZoneComponent"));
	FlashlightDetectionComponent = CreateDefaultSubobject<UFlashlightDetectionComponent>(TEXT("FlashlightDetectionComponent"));

	CloseVisionPointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CloseVisionPointLight"));
	CloseVisionPointLight->SetupAttachment(RootComponent);
	CloseVisionPointLight->SetLightColor(FLinearColor(0.5f, 0.5f, 0.5f));
	CloseVisionPointLight->Intensity = CloseVisionIntensity;
	CloseVisionPointLight->AttenuationRadius = CloseVisionRadius;
	CloseVisionPointLight->CastShadows = false;

	CloseVisionPointLight->SetVisibility(false);
}

void AThiefCharacter::BeginPlay()
{
	Super::BeginPlay();

	InteractSphereComp->OnCanInteract.BindUObject(this, &AThiefCharacter::CheckCanInteract);
	InteractSphereComp->OnGetAbilityTag.BindUObject(this, &AThiefCharacter::ResolveInteractAbilityTag);
}

void AThiefCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (IsLocallyControlled())
	{
		if (IsValid(FlashlightDetectionComponent))
		{
			FlashlightDetectionComponent->StartDetection();
		}

		TryBindZoneTagListener();
	}
}

void AThiefCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (IsLocallyControlled())
	{
		if (IsValid(FlashlightDetectionComponent))
		{
			FlashlightDetectionComponent->StartDetection();
		}

		TryBindZoneTagListener();
	}
}

void AThiefCharacter::UnPossessed()
{
	if (IsValid(FlashlightDetectionComponent))
	{
		FlashlightDetectionComponent->StopDetection();
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (IsValid(ASC) && ZoneTagListenerHandle.IsValid())
	{
		ASC->RegisterGameplayTagEvent(HeistStateTags::Zone_Indoor, EGameplayTagEventType::NewOrRemoved).Remove(ZoneTagListenerHandle);
		ZoneTagListenerHandle.Reset();
	}

	Super::UnPossessed();
}

void AThiefCharacter::ReportFootstep()
{
	if (!IsValid(NoiseComponent)) return;

	if (GetVelocity().IsNearlyZero()) return;

	NoiseComponent->MakeHeistNoise(EHeistSoundType::Footstep_Thief, GetActorLocation());
}

bool AThiefCharacter::CheckCanInteract(ACharacter* Interactor) const
{
	// 자신이 Cuffed 또는 Injured 상태일 때만 상호작용 허용
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && (ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed)
		|| ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured));
}

FGameplayTag AThiefCharacter::ResolveInteractAbilityTag(ACharacter* Interactor) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bCuffed = ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed);
	const bool bInjured = ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured);
	const bool bIsThief = Interactor->IsA<AThiefCharacter>();

	// 상호작용 주체가 도둑일 때
	if (bCuffed && bIsThief) return HeistAbilityTags::Ability_Thief_HelpCuffed;
	if (bInjured && bIsThief) return HeistAbilityTags::Ability_Thief_Heal;

	// 상호작용 주체가 경찰일 때	
	if (bCuffed && !bIsThief) return HeistAbilityTags::Ability_Police_Escort;
	if (bInjured && !bIsThief) return HeistAbilityTags::Ability_Police_Cuffing;

	return FGameplayTag::EmptyTag;
}

void AThiefCharacter::TryBindZoneTagListener()
{
	GetWorld()->GetTimerManager().ClearTimer(ASCBindTimerHandle);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || ASC->GetAvatarActor() != this)
	{
		UWorld* World = GetWorld();
		if (IsValid(World))
		{
			constexpr float RetryDelay = 0.1f;
			World->GetTimerManager().SetTimer(ASCBindTimerHandle, this, &AThiefCharacter::TryBindZoneTagListener, RetryDelay, false);
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
	).AddUObject(this, &AThiefCharacter::OnZoneTagChanged);

	if (IsValid(CloseVisionPointLight))
	{
		const bool bIsIndoor = ASC->HasMatchingGameplayTag(HeistStateTags::Zone_Indoor);
		CloseVisionPointLight->SetVisibility(bIsIndoor);
	}
}

void AThiefCharacter::OnZoneTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (!IsValid(CloseVisionPointLight)) return;

	const bool bIsIndoor = (NewCount > 0);
	CloseVisionPointLight->SetVisibility(bIsIndoor);
}
