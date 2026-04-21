#include "Components/FlashlightComponent.h"

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
	UAbilitySystemComponent* ThiefASC = Thief->GetAbilitySystemComponent();
	if (!IsValid(ThiefASC)) return false;

	// 야외에 있는 도둑은 항상 보임
	if (ThiefASC->HasMatchingGameplayTag(HeistStateTags::Zone_Outdoor))
	{
		return true;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return false;

	FVector PoliceLoc = OwnerActor->GetActorLocation();
	FVector ThiefLoc = Thief->GetActorLocation();

	FVector PoliceLoc2D = FVector(PoliceLoc.X, PoliceLoc.Y, 0.f);
	FVector ThiefLoc2D = FVector(ThiefLoc.X, ThiefLoc.Y, 0.f);

	if (FVector::Distance(PoliceLoc2D, ThiefLoc2D) > FlashlightRadius)
	{
		return false;
	}

	FVector DirectionToThief = (ThiefLoc2D - PoliceLoc2D).GetSafeNormal();
	FVector PoliceForward = OwnerActor->GetActorForwardVector();
	PoliceForward.Z = 0.f;
	PoliceForward.Normalize();

	const float EffectiveHalfAngle = bWasPreviouslyVisible ? (FlashlightHalfAngle + FlashlightHysteresisAngle) : FlashlightHalfAngle;
	const float CosineThreshold = FMath::Cos(FMath::DegreesToRadians(EffectiveHalfAngle));

	if (FVector::DotProduct(PoliceForward, DirectionToThief) < CosineThreshold)
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		PoliceLoc,
		ThiefLoc,
		ECC_Visibility,
		QueryParams
	);

	if (bHit && HitResult.GetActor() != Thief)
	{
		return false;
	}

	return true;
}

void UFlashlightComponent::ProcessLocalVision()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	TArray<TObjectPtr<AThiefCharacter>> CurrentlyVisibleThieves;
	TArray<TObjectPtr<AThiefCharacter>> CurrentlyThievesInCone;

	for (TActorIterator<AThiefCharacter> It(GetWorld()); It; ++It)
	{
		AThiefCharacter* Thief = *It;
		if (!IsValid(Thief)) continue;

		UAbilitySystemComponent* ThiefASC = Thief->GetAbilitySystemComponent();
		if (!IsValid(ThiefASC)) continue;

		// 1. 손전등 (Cone) 검사
		const bool bWasInCone = PreviouslyThievesInCone.Contains(Thief);
		const bool bIsInCone = IsThiefInFlashlight(Thief, bWasInCone);

		// 2. 근접 (원형) 검사: 이송 중이거나 뒤통수를 치러오는 도둑 판단용
		const float DistanceSq = FVector::DistSquaredXY(OwnerActor->GetActorLocation(), Thief->GetActorLocation());
		const bool bInCloseVision = (DistanceSq <= (CloseVisionRadius * CloseVisionRadius));

		const bool bWasVisible = PreviouslyVisibleThieves.Contains(Thief);
		const bool bIsNowVisible = bIsInCone || bInCloseVision;

		if (bIsNowVisible != bWasVisible)
		{
			UHeistTransparencyComponent* TransComp = Thief->GetComponentByClass<UHeistTransparencyComponent>();
			if (IsValid(TransComp))
			{
				TransComp->SetTargetVisibility(bIsNowVisible);
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
				ThiefASC->AddLooseGameplayTag(HeistStateTags::State_Thief_InFlashlight);
			}
			else
			{
				OnThiefLost.Broadcast(Thief);
				ThiefASC->RemoveLooseGameplayTag(HeistStateTags::State_Thief_InFlashlight);
			}
		}

		if (bIsNowVisible) CurrentlyVisibleThieves.Add(Thief);
		if (bIsInCone) CurrentlyThievesInCone.Add(Thief);
	}

	PreviouslyVisibleThieves = CurrentlyVisibleThieves;
	PreviouslyThievesInCone = CurrentlyThievesInCone;
}
