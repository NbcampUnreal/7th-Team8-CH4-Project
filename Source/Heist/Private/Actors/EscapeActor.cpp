#include "Actors/EscapeActor.h"
#include "Components/BoxComponent.h"
#include "Components/HeistInteractSphereComponent.h"
#include "Core/HeistMatchGameState.h"
#include "Net/UnrealNetwork.h"

#include "AbilitySystem/HeistTags_Ability.h"
#include "AbilitySystemComponent.h"

AEscapeActor::AEscapeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	SetRootComponent(BoxCollision);

	BoxCollision->SetMobility(EComponentMobility::Static);
	BoxCollision->SetCollisionProfileName(TEXT("Pawn"));
	BoxCollision->SetGenerateOverlapEvents(true);
	BoxCollision->ComponentTags.Add(FName("MainBody"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(BoxCollision);
	Mesh->SetMobility(EComponentMobility::Static);

	InteractSphereComponent = CreateDefaultSubobject<UHeistInteractSphereComponent>(TEXT("InteractSphereComponent"));
}

void AEscapeActor::BeginPlay()
{
	Super::BeginPlay();

	InteractSphereComponent->OnCanInteract.BindUObject(this, &AEscapeActor::CheckCanInteract);
	InteractSphereComponent->OnGetAbilityTag.BindUObject(this, &AEscapeActor::ResolveInteractAbilityTag);
}

void AEscapeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEscapeActor, bCanEscape);
}

bool AEscapeActor::CheckCanInteract(ACharacter* Interactor) const
{
	AHeistMatchGameState* HeistGS = GetWorld()->GetGameState<AHeistMatchGameState>();
	if (!HeistGS) return false;

	if (!HeistGS->IsEngineChannelingStarted() && bCanEscape) return true;
	else if (HeistGS->IsEngineChannelingEnded()) return true;

	return false;
}

FGameplayTag AEscapeActor::ResolveInteractAbilityTag(ACharacter* Interactor) const
{
	AHeistMatchGameState* HeistGS = GetWorld()->GetGameState<AHeistMatchGameState>();
	if (!HeistGS) return FGameplayTag::EmptyTag;

	if (!HeistGS->IsEngineChannelingStarted() && bCanEscape) return HeistAbilityTags::Ability_Thief_StartEngine;
	else if (HeistGS->IsEngineChannelingEnded()) return HeistAbilityTags::Ability_Thief_Depart;

	return FGameplayTag::EmptyTag;
}

void AEscapeActor::SetActivation(bool bIsActive)
{
	bCanEscape = bIsActive;

	if (bCanEscape)
	{
		UE_LOG(LogTemp, Warning, TEXT("탈출구가 활성화되었습니다!"));
	}
}
