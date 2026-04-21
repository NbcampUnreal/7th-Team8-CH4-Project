#include "Components/HeistTransparencyComponent.h"

#include "Character/PoliceCharacter.h"

#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

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

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	TArray<UMeshComponent*> RawMeshComponents;
	OwnerActor->GetComponents<UMeshComponent>(RawMeshComponents);
	for (UMeshComponent* Mesh : RawMeshComponents)
	{
		if (IsValid(Mesh))
		{
			CachedMeshComponents.Add(Mesh);
		}
	}

	APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
	if (IsValid(PC))
	{
		APawn* LocalPawn = PC->GetPawn();
		bCachedIsLocalViewerPolice = IsValid(LocalPawn) && LocalPawn->IsA<APoliceCharacter>();
	}

	if (bCachedIsLocalViewerPolice && bStartInvisibleToPolice)
	{
		bIsTargetVisible = false;
		CurrentOpacity = 0.0f;
		SetAllMeshVisibility(false);
	}
	else
	{
		bIsTargetVisible = true;
		CurrentOpacity = 1.0f;
		SetAllMeshVisibility(true);
	}

	for (UMeshComponent* Mesh : CachedMeshComponents)
	{
		const int32 MaterialCount = Mesh->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
			if (IsValid(DynamicMaterial))
			{
				DynamicMaterial->SetScalarParameterValue(OpacityParamName, CurrentOpacity);
				DynamicMaterials.Add(DynamicMaterial);
			}
		}
	}
}

void UHeistTransparencyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float TargetOpacity = bIsTargetVisible ? 1.0f : 0.0f;
	CurrentOpacity = FMath::FInterpConstantTo(CurrentOpacity, TargetOpacity, DeltaTime, FadeSpeed);

	for (UMaterialInstanceDynamic* DynamicMaterial : DynamicMaterials)
	{
		if (IsValid(DynamicMaterial))
		{
			DynamicMaterial->SetScalarParameterValue(OpacityParamName, CurrentOpacity);
		}
	}

	const bool bReachedTarget = FMath::IsNearlyEqual(CurrentOpacity, TargetOpacity, OpacityNearlyEqualTolerance);
	if (bReachedTarget)
	{
		CurrentOpacity = TargetOpacity;
		SetComponentTickEnabled(false);

		if (CurrentOpacity <= 0.0f)
		{
			SetAllMeshVisibility(false);
		}
	}
}

void UHeistTransparencyComponent::SetTargetVisibility(bool bVisible)
{
	if (!IsLocalViewerPolice()) return;
	if (bIsTargetVisible == bVisible) return;

	bIsTargetVisible = bVisible;

	if (bIsTargetVisible)
	{
		SetAllMeshVisibility(true);
	}

	SetComponentTickEnabled(true);
}

void UHeistTransparencyComponent::ForceRestoreVisibility()
{
	bHasCachedViewer = false;
	bCachedIsLocalViewerPolice = false;
	bIsTargetVisible = true;
	CurrentOpacity = 1.0f;
	SetAllMeshVisibility(true);

	for (UMaterialInstanceDynamic* DynamicMaterial : DynamicMaterials)
	{
		if (IsValid(DynamicMaterial))
		{
			DynamicMaterial->SetScalarParameterValue(OpacityParamName, 1.0f);
		}
	}

	SetComponentTickEnabled(false);
}

bool UHeistTransparencyComponent::IsLocalViewerPolice()
{
	// 이미 성공적으로 캐싱했다면 캐시값 반환
	if (bHasCachedViewer) return bCachedIsLocalViewerPolice;

	APlayerController* PlayerController = GEngine->GetFirstLocalPlayerController(GetWorld());
	if (IsValid(PlayerController))
	{
		APawn* LocalPawn = PlayerController->GetPawn();
		// 로컬 폰이 유효한 상태(빙의 완료)일 때만 캐싱을 확정합니다.
		if (IsValid(LocalPawn))
		{
			bCachedIsLocalViewerPolice = LocalPawn->IsA<APoliceCharacter>();
			bHasCachedViewer = true;
			return bCachedIsLocalViewerPolice;
		}
	}
	// 빙의 전이라면 일단 false를 반환하고 다음 번에 다시 시도합니다.
	return false;
}

void UHeistTransparencyComponent::SetAllMeshVisibility(bool bVisible)
{
	for (UMeshComponent* Mesh : CachedMeshComponents)
	{
		if (IsValid(Mesh))
		{
			Mesh->SetVisibility(bVisible, true);
		}
	}
}
