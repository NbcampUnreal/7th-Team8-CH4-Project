#include "Character/ThiefCharacter.h"

#include "AbilitySystem/HeistTags_Ability.h"
#include "Character/PoliceCharacter.h"
#include "Character/HeistTags_State.h"
#include "Components/HeistInteractSphereComponent.h"
#include "Components/ThiefEscortComponent.h"
#include "Components/HeistZoneComponent.h"
#include "Components/HeistNoiseComponent.h"
#include "Data/HeistSoundData.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AThiefCharacter::AThiefCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCharacterMovement()->RotationRate = FRotator(0.f, MaxRotationRateYaw, 0.f);

	EscortComponent = CreateDefaultSubobject<UThiefEscortComponent>(TEXT("EscortComponent"));
	InteractSphereComp = CreateDefaultSubobject<UHeistInteractSphereComponent>(TEXT("InteractSphereComp"));
	NoiseComponent = CreateDefaultSubobject<UHeistNoiseComponent>(TEXT("NoiseComponent"));
	ZoneComponent = CreateDefaultSubobject<UHeistZoneComponent>(TEXT("ZoneComponent"));
}

void AThiefCharacter::BeginPlay()
{
	Super::BeginPlay();

	InteractSphereComp->OnCanInteract.BindUObject(this, &AThiefCharacter::CheckCanInteract);
	InteractSphereComp->OnGetAbilityTag.BindUObject(this, &AThiefCharacter::ResolveInteractAbilityTag);

	APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
	if (IsValid(PC))
	{
		APawn* LocalPawn = PC->GetPawn();
		if (IsValid(LocalPawn) && LocalPawn->IsA<APoliceCharacter>())
		{
			GetMesh()->SetVisibility(false, true);
		}
	}
}

void AThiefCharacter::ReportFootstep()
{
	if (!IsValid(NoiseComponent)) return;

	if (GetVelocity().IsNearlyZero()) return;

	NoiseComponent->MakeHeistNoise(EHeistSoundType::Footstep, GetActorLocation());
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
