#include "Actors/DropZoneVolume.h"

#include "Actors/ItemActor.h"
#include "Components/BoxComponent.h"

ADropZoneVolume::ADropZoneVolume() : CurrentValue(0)
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;

}

void ADropZoneVolume::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ADropZoneVolume::OnOverlapBegin);
		CollisionBox->OnComponentEndOverlap.AddDynamic(this, &ADropZoneVolume::OnOverlapEnd);
	}
}

void ADropZoneVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	AItemActor* Item = Cast<AItemActor>(OtherActor);
	if (!IsValid(Item)) return;
	if (!OtherComp->ComponentHasTag(FName("MainBody"))) return;

	int32 ItemValue = Item->GetItemValue();
	CurrentValue += ItemValue;
}

void ADropZoneVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;

	AItemActor* Item = Cast<AItemActor>(OtherActor);
	if (!IsValid(Item)) return;
	if (!OtherComp->ComponentHasTag(FName("MainBody"))) return;

	int32 ItemValue = Item->GetItemValue();
	CurrentValue -= ItemValue;
}
