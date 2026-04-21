#include "Components/FlashlightComponent.h"

#include "Actors/ItemActor.h"
#include "Character/ThiefCharacter.h"
#include "Character/HeistTags_State.h"
#include "Components/HeistTransparencyComponent.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "TimerManager.h"

UFlashlightComponent::UFlashlightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlashlightComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFlashlightComponent::TryStartLocalVision()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn) || !OwnerPawn->IsLocallyControlled()) return;

	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(VisionCheckTimerHandle)) return;

	World->GetTimerManager().SetTimer(VisionCheckTimerHandle, this, &UFlashlightComponent::ProcessLocalVision, VisionCheckInterval, true);
}

void UFlashlightComponent::StopLocalVision()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(VisionCheckTimerHandle);
	}
}

bool UFlashlightComponent::IsThiefInFlashlight(AThiefCharacter* Thief, bool bWasPreviouslyVisible) const
{
	UAbilitySystemComponent* ThiefAbilitySystemComponent = Thief->GetAbilitySystemComponent();
	if (!IsValid(ThiefAbilitySystemComponent)) return false;

	// 야외에 있는 도둑은 항상 보임
	if (ThiefAbilitySystemComponent->HasMatchingGameplayTag(HeistStateTags::Zone_Outdoor)) return true;

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return false;

	FVector PoliceLocation = OwnerActor->GetActorLocation();
	FVector ThiefLocation = Thief->GetActorLocation();

	FVector PoliceLocation2D = FVector(PoliceLocation.X, PoliceLocation.Y, 0.f);
	FVector ThiefLocation2D = FVector(ThiefLocation.X, ThiefLocation.Y, 0.f);

	if (FVector::Distance(PoliceLocation2D, ThiefLocation2D) > FlashlightRadius) return false;

	FVector DirectionToThief = (ThiefLocation2D - PoliceLocation2D).GetSafeNormal();
	FVector PoliceForward = OwnerActor->GetActorForwardVector();
	PoliceForward.Z = 0.f;
	PoliceForward.Normalize();

	const float EffectiveHalfAngle = bWasPreviouslyVisible
		? (FlashlightHalfAngle + FlashlightHysteresisAngle)
		: FlashlightHalfAngle;
	const float CosineThreshold = FMath::Cos(FMath::DegreesToRadians(EffectiveHalfAngle));

	if (FVector::DotProduct(PoliceForward, DirectionToThief) < CosineThreshold)	return false;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	const bool bHitSomething = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		PoliceLocation,
		ThiefLocation,
		ECC_Visibility,
		QueryParams
	);

	if (!bHitSomething) return true;
	return HitResult.GetActor() == Thief;
}

bool UFlashlightComponent::IsItemInFlashlight(AItemActor* Item, const FVector& PoliceLocation) const
{
	if (!IsValid(Item)) return false;

	const FVector ItemLocation = Item->GetActorLocation();

	const FVector PoliceLocation2D = FVector(PoliceLocation.X, PoliceLocation.Y, 0.f);
	const FVector ItemLocation2D = FVector(ItemLocation.X, ItemLocation.Y, 0.f);

	if (FVector::Distance(PoliceLocation2D, ItemLocation2D) > FlashlightRadius) return false;

	const FVector DirectionToItem = (ItemLocation2D - PoliceLocation2D).GetSafeNormal();

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return false;

	FVector PoliceForward = OwnerActor->GetActorForwardVector();
	PoliceForward.Z = 0.f;
	PoliceForward.Normalize();

	const float CosineThreshold = FMath::Cos(FMath::DegreesToRadians(FlashlightHalfAngle));
	if (FVector::DotProduct(PoliceForward, DirectionToItem) < CosineThreshold) return false;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	const bool bHitSomething = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		PoliceLocation,
		ItemLocation,
		ECC_Visibility,
		QueryParams
	);

	if (!bHitSomething) return true;

	AActor* HitActor = HitResult.GetActor();

	return HitActor == Item || (IsValid(HitActor) && HitActor->IsA<AThiefCharacter>());
}

void UFlashlightComponent::ProcessLocalVision()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	const FVector PoliceLocation = OwnerActor->GetActorLocation();

	TArray<TObjectPtr<AThiefCharacter>> CurrentlyVisibleThieves;
	TArray<TObjectPtr<AThiefCharacter>> CurrentlyThievesInCone;

	// 1. 도둑 가시성 판정 로직
	for (TActorIterator<AThiefCharacter> It(GetWorld()); It; ++It)
	{
		AThiefCharacter* Thief = *It;
		if (!IsValid(Thief)) continue;

		UAbilitySystemComponent* ThiefAbilitySystemComponent = Thief->GetAbilitySystemComponent();
		if (!IsValid(ThiefAbilitySystemComponent)) continue;

		// 1. 손전등 (Cone) 검사
		const bool bWasInCone = PreviouslyThievesInCone.Contains(Thief);
		const bool bIsInCone = IsThiefInFlashlight(Thief, bWasInCone);

		// 2. 근접 (원형) 검사: 이송 중이거나 뒤통수를 치러오는 도둑 판단용
		const float DistanceSquared = FVector::DistSquaredXY(PoliceLocation, Thief->GetActorLocation());
		const bool bInCloseVision = (DistanceSquared <= FMath::Square(CloseVisionRadius));

		const bool bWasVisible = PreviouslyVisibleThieves.Contains(Thief);
		const bool bIsNowVisible = bIsInCone || bInCloseVision;

		if (bIsNowVisible != bWasVisible)
		{
			UHeistTransparencyComponent* TransparencyComponent = Thief->GetComponentByClass<UHeistTransparencyComponent>();
			if (IsValid(TransparencyComponent))
			{
				TransparencyComponent->SetTargetVisibility(bIsNowVisible);
			}
			else if (USkeletalMeshComponent* ThiefMesh = Thief->GetMesh())
			{
				ThiefMesh->SetVisibility(bIsNowVisible, true);
			}
		}

		if (bIsInCone != bWasInCone)
		{
			if (bIsInCone)
			{
				OnThiefSpotted.Broadcast(Thief);
				ThiefAbilitySystemComponent->AddLooseGameplayTag(HeistStateTags::State_Thief_InFlashlight);
			}
			else
			{
				OnThiefLost.Broadcast(Thief);
				ThiefAbilitySystemComponent->RemoveLooseGameplayTag(HeistStateTags::State_Thief_InFlashlight);
			}
		}

		if (bIsNowVisible) CurrentlyVisibleThieves.Add(Thief);
		if (bIsInCone) CurrentlyThievesInCone.Add(Thief);
	}

	// 2. 운반 중인 물건 가시성 판정 로직
	for (TActorIterator<AItemActor> It(GetWorld()); It; ++It)
	{
		AItemActor* Item = *It;
		if (!IsValid(Item)) continue;
		if (Item->GetCurrentCarrierCount() <= 0) continue;

		const bool bIsInCone = IsItemInFlashlight(Item, PoliceLocation);

		const float DistanceSquared = FVector::DistSquaredXY(PoliceLocation, Item->GetActorLocation());
		const bool bInCloseVision = (DistanceSquared <= FMath::Square(CloseVisionRadius));

		const bool bIsNowVisible = bIsInCone || bInCloseVision;

		UHeistTransparencyComponent* TransparencyComponent = Item->GetComponentByClass<UHeistTransparencyComponent>();
		if (IsValid(TransparencyComponent))
		{
			TransparencyComponent->SetTargetVisibility(bIsNowVisible);
		}
	}

	PreviouslyVisibleThieves = CurrentlyVisibleThieves;
	PreviouslyThievesInCone = CurrentlyThievesInCone;
}
