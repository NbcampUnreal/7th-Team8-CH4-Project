#include "Components/FlashlightComponent.h"

#include "Actors/ItemActor.h"
#include "Character/ThiefCharacter.h"
#include "Character/HeistTags_State.h"
#include "Components/HeistTransparencyComponent.h"

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

void UFlashlightComponent::RestoreAllThiefVisibility()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	for (TActorIterator<AThiefCharacter> It(World); It; ++It)
	{
		AThiefCharacter* Thief = *It;
		if (!IsValid(Thief)) continue;

		UHeistTransparencyComponent* TransparencyComponent = Thief->GetComponentByClass<UHeistTransparencyComponent>();
		if (IsValid(TransparencyComponent))
		{
			TransparencyComponent->SetTargetVisibility(true);
		}
		else if (USkeletalMeshComponent* ThiefMesh = Thief->GetMesh())
		{
			ThiefMesh->SetVisibility(true, true);
		}
	}

	PreviouslyVisibleThieves.Reset();
	PreviouslyThievesInCone.Reset();
}

bool UFlashlightComponent::IsTargetInFlashlightCone(AActor* TargetActor, const FVector& PoliceLocation, float EffectiveHalfAngle) const
{
	if (!IsValid(TargetActor)) return false;

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return false;

	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector PoliceLocation2D(PoliceLocation.X, PoliceLocation.Y, 0.f);
	const FVector TargetLocation2D(TargetLocation.X, TargetLocation.Y, 0.f);

	// 1. 거리 체크
	if (FVector::Distance(PoliceLocation2D, TargetLocation2D) > FlashlightRadius) return false;

	// 2. 각도 체크
	const FVector DirectionToTarget = (TargetLocation2D - PoliceLocation2D).GetSafeNormal();
	FVector PoliceForward = OwnerActor->GetActorForwardVector();
	PoliceForward.Z = 0.f;
	PoliceForward.Normalize();

	const float CosineThreshold = FMath::Cos(FMath::DegreesToRadians(EffectiveHalfAngle));
	if (FVector::DotProduct(PoliceForward, DirectionToTarget) < CosineThreshold) return false;

	// 3. LineTrace 체크
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	const bool bHitSomething = GetWorld()->LineTraceSingleByChannel(HitResult, PoliceLocation, TargetLocation, ECC_Visibility, QueryParams);

	if (!bHitSomething) return true;

	AActor* HitActor = HitResult.GetActor();
	return HitActor == TargetActor || (IsValid(HitActor) && HitActor->IsA<AThiefCharacter>());
}

bool UFlashlightComponent::IsThiefInFlashlight(AThiefCharacter* Thief, bool bWasPreviouslyVisible) const
{
	UAbilitySystemComponent* ThiefAbilitySystemComponent = Thief->GetAbilitySystemComponent();
	if (!IsValid(ThiefAbilitySystemComponent)) return false;

	const float EffectiveHalfAngle = bWasPreviouslyVisible ? (FlashlightHalfAngle + FlashlightHysteresisAngle) : FlashlightHalfAngle;

	return IsTargetInFlashlightCone(Thief, GetOwner()->GetActorLocation(), EffectiveHalfAngle);
}

bool UFlashlightComponent::IsItemInFlashlight(AItemActor* Item, const FVector& PoliceLocation) const
{
	return IsTargetInFlashlightCone(Item, PoliceLocation, FlashlightHalfAngle);
}

// TODO(하민): 추후 스폰된 도둑과 아이템 목록을 캐싱(Caching)해두고 그 배열을 순회하는 방식으로 최적화
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

		const bool bIsOutdoor = ThiefAbilitySystemComponent->HasMatchingGameplayTag(HeistStateTags::Zone_Outdoor);
		const bool bWasVisible = PreviouslyVisibleThieves.Contains(Thief);
		const bool bIsNowVisible = bIsInCone || bInCloseVision || bIsOutdoor;

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
