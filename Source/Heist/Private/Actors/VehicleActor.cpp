#include "Actors/VehicleActor.h"
#include "Components/BoxComponent.h"
#include "Components/HeistInteractSphereComponent.h"
#include "Net/UnrealNetwork.h"

#include "Core/HeistMatchGameState.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "AbilitySystemComponent.h"

AVehicleActor::AVehicleActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	InteractSphereComponent = CreateDefaultSubobject<UHeistInteractSphereComponent>(TEXT("InteractSphereComponent"));
}

void AVehicleActor::SetDoorOpened(bool bIsDoorOpened)
{
	bDoorOpened = bIsDoorOpened;
}

void AVehicleActor::BeginPlay()
{
	Super::BeginPlay();

	InteractSphereComponent->OnCanInteract.BindUObject(this, &AVehicleActor::CheckCanInteract);
	InteractSphereComponent->OnGetAbilityTag.BindUObject(this, &AVehicleActor::ResolveInteractAbilityTag);
}

void AVehicleActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVehicleActor, bDoorOpened);
}

bool AVehicleActor::CheckCanInteract(ACharacter* Interactor) const
{
	AHeistMatchGameState* HeistGS = GetWorld()->GetGameState<AHeistMatchGameState>();
	if (!HeistGS) return false;

	if (HeistGS->IsEngineChannelingStarted()) return true;

	return false;
}

FGameplayTag AVehicleActor::ResolveInteractAbilityTag(ACharacter* Interactor) const
{
	AHeistMatchGameState* HeistGS = GetWorld()->GetGameState<AHeistMatchGameState>();
	if (!HeistGS) return FGameplayTag::EmptyTag;

	if (!HeistGS->IsEngineChannelingStarted()) return FGameplayTag::EmptyTag;

	if(bDoorOpened) return HeistAbilityTags::Ability_Thief_CloseDoor;
	else return HeistAbilityTags::Ability_Police_OpenDoor;

	return FGameplayTag::EmptyTag;
}
