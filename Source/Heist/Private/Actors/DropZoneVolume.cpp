#include "Actors/DropZoneVolume.h"

#include "Actors/ItemActor.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

ADropZoneVolume::ADropZoneVolume() : CurrentValue(0), ZoneIndex(0)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

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

void ADropZoneVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADropZoneVolume, CurrentValue);
	DOREPLIFETIME(ADropZoneVolume, ZoneIndex);
}

void ADropZoneVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	AItemActor* Item = Cast<AItemActor>(OtherActor);
	if (!IsValid(Item)) return;
	if (!OtherComp->ComponentHasTag(FName("MainBody"))) return;

	int32 ItemValue = Item->GetItemValue();
	CurrentValue += ItemValue;

	OnZoneValueChanged.Broadcast(ZoneIndex, CurrentValue);
}

void ADropZoneVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;

	AItemActor* Item = Cast<AItemActor>(OtherActor);
	if (!IsValid(Item)) return;
	if (!OtherComp->ComponentHasTag(FName("MainBody"))) return;

	int32 ItemValue = Item->GetItemValue();
	CurrentValue -= ItemValue;

	OnZoneValueChanged.Broadcast(ZoneIndex, CurrentValue);
}
