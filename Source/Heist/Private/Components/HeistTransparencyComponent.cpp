#include "Components/HeistTransparencyComponent.h"

#include "Character/PoliceCharacter.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

UHeistTransparencyComponent::UHeistTransparencyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bIsTargetVisible = true;
	CurrentOpacity = 0.0f;
}

void UHeistTransparencyComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!IsValid(OwnerCharacter)) return;

	USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh();
	if (!IsValid(MeshComp)) return;

	bool bIsLocalViewerPolice = false;
	APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
	if (IsValid(PC))
	{
		APawn* LocalPawn = PC->GetPawn();
		if (IsValid(LocalPawn) && LocalPawn->IsA<APoliceCharacter>())
		{
			bIsLocalViewerPolice = true;
		}
	}

	if (bIsLocalViewerPolice)
	{
		bIsTargetVisible = false;
		CurrentOpacity = 0.0f;
		MeshComp->SetVisibility(false, true);
	}
	else
	{
		bIsTargetVisible = true;
		CurrentOpacity = 1.0f;
		MeshComp->SetVisibility(true, true);
	}

	const int32 MaterialCount = MeshComp->GetNumMaterials();
	for (int32 i = 0; i < MaterialCount; ++i)
	{
		UMaterialInstanceDynamic* MID = MeshComp->CreateAndSetMaterialInstanceDynamic(i);
		if (IsValid(MID))
		{
			MID->SetScalarParameterValue(OpacityParamName, CurrentOpacity);
			DynamicMaterials.Add(MID);
		}
	}
}

void UHeistTransparencyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float TargetOpacity = bIsTargetVisible ? 1.0f : 0.0f;

	CurrentOpacity = FMath::FInterpConstantTo(CurrentOpacity, TargetOpacity, DeltaTime, FadeSpeed);

	for (UMaterialInstanceDynamic* MID : DynamicMaterials)
	{
		if (IsValid(MID))
		{
			MID->SetScalarParameterValue(OpacityParamName, CurrentOpacity);
		}
	}

	const bool bReachedTarget = FMath::IsNearlyEqual(CurrentOpacity, TargetOpacity, 0.01f);
	if (bReachedTarget)
	{
		CurrentOpacity = TargetOpacity;
		SetComponentTickEnabled(false);

		if (CurrentOpacity <= 0.0f)
		{
			ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
			if (IsValid(OwnerCharacter) && IsValid(OwnerCharacter->GetMesh()))
			{
				OwnerCharacter->GetMesh()->SetVisibility(false, true);
			}
		}
	}
}

void UHeistTransparencyComponent::SetTargetVisibility(bool bVisible)
{
	APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
	if (IsValid(PC))
	{
		APawn* LocalPawn = PC->GetPawn();
		if (IsValid(LocalPawn) && !LocalPawn->IsA<APoliceCharacter>()) return;
	}

	if (bIsTargetVisible == bVisible) return;

	bIsTargetVisible = bVisible;

	if (bIsTargetVisible)
	{
		ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (IsValid(OwnerCharacter) && IsValid(OwnerCharacter->GetMesh()))
		{
			OwnerCharacter->GetMesh()->SetVisibility(true, true);
		}
	}

	SetComponentTickEnabled(true);
}
