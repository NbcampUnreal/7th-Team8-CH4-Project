#include "Actors/ItemActor.h"

#include "Components/BoxComponent.h"
#include "AbilitySystem/HeistTags_FlagTags.h"
#include "Net/UnrealNetwork.h"

AItemActor::AItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;


	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetupAttachment(GetRootComponent());

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(BoxCollision);
}

void AItemActor::BeginPlay()
{
	Super::BeginPlay();

}

bool AItemActor::CanInteract_Implementation(ACharacter* Interactor) const
{
	// 기본적으로 Interact 가능, 필요시 로직 추가
	return true;
}

FGameplayTag AItemActor::GetInteractAbilityTag_Implementation(ACharacter* Interactor) const
{
	return HeistFlagTags::Tag_SoloCarrying;
}

float AItemActor::GetInteractRadius_Implementation() const
{
	return 0.0f;
}

void AItemActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bIsCarried);
}

void AItemActor::OnRep_IsCarried()
{
}

