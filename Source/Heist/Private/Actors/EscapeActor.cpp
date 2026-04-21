#include "Actors/EscapeActor.h"
#include "Components/BoxComponent.h"
#include "Components/HeistInteractSphereComponent.h"

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
	BoxCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
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
	
}

bool AEscapeActor::CheckCanInteract(ACharacter* Interactor) const
{
	// 해당 DropZone의 점수가 목표점수 이상이면
	return true;
}

FGameplayTag AEscapeActor::ResolveInteractAbilityTag(ACharacter* Interactor) const
{

	return HeistAbilityTags::Ability_Thief_CloseDoor;
}
