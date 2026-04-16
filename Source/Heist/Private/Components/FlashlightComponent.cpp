#include "Components/FlashlightComponent.h"

#include "Character/ThiefCharacter.h"
#include "Character/HeistTags_State.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "TimerManager.h"

// TODO(하민): Material 적용할 때 디버깅 라인 제거
#if !UE_BUILD_SHIPPING
#include "DrawDebugHelpers.h"
#endif

UFlashlightComponent::UFlashlightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlashlightComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled())
	{
		GetWorld()->GetTimerManager().SetTimer(VisionCheckTimerHandle, this, &UFlashlightComponent::ProcessLocalVision, 0.1f, true);
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

	// TODO(하민): Material 적용할 때 디버깅 라인 제거
#if !UE_BUILD_SHIPPING
	constexpr int32 DebugConeSegments = 16;
	constexpr float DebugConeLifeTime = 0.15f;

	DrawDebugCone(
		GetWorld(),
		OwnerActor->GetActorLocation(),
		OwnerActor->GetActorForwardVector(),
		FlashlightRadius,
		FMath::DegreesToRadians(FlashlightHalfAngle),
		FMath::DegreesToRadians(FlashlightHalfAngle),
		DebugConeSegments,
		FColor::Yellow,
		false,
		DebugConeLifeTime,
		0,
		1.0f
	);
#endif

	TSet<AThiefCharacter*> CurrentlyVisibleThieves;

	for (TActorIterator<AThiefCharacter> It(GetWorld()); It; ++It)
	{
		AThiefCharacter* Thief = *It;
		if (!IsValid(Thief)) continue;

		UAbilitySystemComponent* ThiefASC = Thief->GetAbilitySystemComponent();
		if (!IsValid(ThiefASC)) continue;

		const bool bWasPreviouslyVisible = PreviouslyVisibleThieves.Contains(Thief);
		const bool bIsNowVisible = IsThiefInFlashlight(Thief, bWasPreviouslyVisible);

		if (bIsNowVisible != bWasPreviouslyVisible)
		{
			USkeletalMeshComponent* ThiefMesh = Thief->GetMesh();
			if (IsValid(ThiefMesh))
			{
				ThiefMesh->SetVisibility(bIsNowVisible, true);
			}

			if (bIsNowVisible)
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

		if (bIsNowVisible)
		{
			CurrentlyVisibleThieves.Add(Thief);
		}
	}

	const bool bWasAnyVisible = (PreviouslyVisibleThieves.Num() > 0);
	const bool bIsAnyVisible = (CurrentlyVisibleThieves.Num() > 0);

	if (bWasAnyVisible != bIsAnyVisible)
	{
		FHeistFlashlightAlertMessage Message;
		Message.bIsDetected = bIsAnyVisible;

		UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(GetWorld());
		MessageSubsystem.BroadcastMessage(HeistMessageTags::Message_UI_FlashlightAlert, Message);
	}

	PreviouslyVisibleThieves = CurrentlyVisibleThieves;
}
