#include "Components/FlashlightComponent.h"

#include "Character/ThiefCharacter.h"
#include "Character/HeistTags_State.h"
#include "AbilitySystemComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

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
	FVector PoliceLoc = OwnerActor->GetActorLocation();
	FVector ThiefLoc = Thief->GetActorLocation();

	PoliceLoc.Z = 0.f;
	ThiefLoc.Z = 0.f;

	if (FVector::Distance(PoliceLoc, ThiefLoc) > FlashlightRadius)
	{
		return false;
	}

	FVector DirectionToThief = (ThiefLoc - PoliceLoc).GetSafeNormal();
	FVector PoliceForward = OwnerActor->GetActorForwardVector();
	PoliceForward.Z = 0.f;
	PoliceForward.Normalize();

	const float EffectiveHalfAngle = bWasPreviouslyVisible ? (FlashlightHalfAngle + 2.0f) : FlashlightHalfAngle;
	const float CosineThreshold = FMath::Cos(FMath::DegreesToRadians(EffectiveHalfAngle));

	return FVector::DotProduct(PoliceForward, DirectionToThief) >= CosineThreshold;
}

void UFlashlightComponent::ProcessLocalVision()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	// TODO(하민): Material 적용할 때 디버깅 라인 제거
#if !UE_BUILD_SHIPPING
	DrawDebugCone(
		GetWorld(),
		OwnerActor->GetActorLocation(),
		OwnerActor->GetActorForwardVector(),
		FlashlightRadius,
		FMath::DegreesToRadians(FlashlightHalfAngle),
		FMath::DegreesToRadians(FlashlightHalfAngle),
		16,
		FColor::Yellow,
		false,
		0.15f,
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
			if (USkeletalMeshComponent* ThiefMesh = Thief->GetMesh())
			{
				ThiefMesh->SetVisibility(bIsNowVisible, true);
			}

			// 진입/이탈 델리게이트 브로드캐스트 (HUD, 사운드에서 구독)
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

	PreviouslyVisibleThieves = CurrentlyVisibleThieves;
}
