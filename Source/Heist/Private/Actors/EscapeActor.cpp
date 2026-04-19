#include "Actors/EscapeActor.h"
#include "Components/HeistInteractSphereComponent.h"
#include "Components/BoxComponent.h"

AEscapeActor::AEscapeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	SetRootComponent(BoxCollision);

	BoxCollision->SetSimulatePhysics(true);
	BoxCollision->SetCollisionProfileName(TEXT("PhysicsActor"));
	BoxCollision->SetUseCCD(true);
	BoxCollision->ComponentTags.Add(FName("MainBody"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(BoxCollision);

	InteractSphereComponent = CreateDefaultSubobject<UHeistInteractSphereComponent>(TEXT("InteractSphereComponent"));
}

void AEscapeActor::BeginPlay()
{
	Super::BeginPlay();
	
}

