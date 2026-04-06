#include "Components/ThiefEscortComponent.h"

#include "Character/HeistCharacter.h"

#include "Net/UnrealNetwork.h"

UThiefEscortComponent::UThiefEscortComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false);
	SetIsReplicatedByDefault(true);
}

void UThiefEscortComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThiefEscortComponent, EscortedBy);
}

void UThiefEscortComponent::SetEscortedBy(AHeistCharacter* InPolice)
{
	AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) && OwnerActor->HasAuthority())
	{
		EscortedBy = InPolice;
		SetComponentTickEnabled(IsValid(InPolice));
	}
}

void UThiefEscortComponent::OnRep_EscortedBy()
{
	// TODO: 클라이언트 UI 갱신 (수갑 아이콘 등)
}

void UThiefEscortComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || !IsValid(EscortedBy)) return;

	FVector TargetLocation = EscortedBy->GetActorLocation() - (EscortedBy->GetActorForwardVector() * EscortOffsetDistance);
	TargetLocation.Z = EscortedBy->GetActorLocation().Z;

	OwnerActor->SetActorRotation(EscortedBy->GetActorRotation());
	OwnerActor->SetActorLocation(TargetLocation, false);

}
