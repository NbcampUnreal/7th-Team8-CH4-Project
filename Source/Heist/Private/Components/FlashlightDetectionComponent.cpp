#include "Components/FlashlightDetectionComponent.h"

#include "Character/PoliceCharacter.h"
#include "Components/FlashlightComponent.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

UFlashlightDetectionComponent::UFlashlightDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWasInFlashlight = false;
}

void UFlashlightDetectionComponent::StartDetection()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	if (!World->GetTimerManager().IsTimerActive(AlertCheckTimerHandle))
	{
		World->GetTimerManager().SetTimer(AlertCheckTimerHandle, this, &UFlashlightDetectionComponent::ProcessDetectionCheck, DetectionCheckInterval, true);
	}
}

void UFlashlightDetectionComponent::StopDetection()
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(AlertCheckTimerHandle);
	}
	bWasInFlashlight = false;
}

void UFlashlightDetectionComponent::ProcessDetectionCheck()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	bool bAmIInFlashlight = false;
	const FVector MyLoc = OwnerActor->GetActorLocation();

	for (TActorIterator<APoliceCharacter> It(World); It; ++It)
	{
		APoliceCharacter* Police = *It;
		if (!IsValid(Police)) continue;

		UFlashlightComponent* FlashlightComp = Police->GetFlashlightComponent();
		if (!IsValid(FlashlightComp)) continue;

		const float Radius = FlashlightComp->GetFlashlightRadius();
		const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(FlashlightComp->GetFlashlightHalfAngle()));

		const FVector PoliceLoc = Police->GetActorLocation();

		FVector PoliceLoc2D = FVector(PoliceLoc.X, PoliceLoc.Y, 0.f);
		FVector MyLoc2D = FVector(MyLoc.X, MyLoc.Y, 0.f);

		if (FVector::Distance(PoliceLoc, MyLoc) > Radius) continue;

		FVector DirectionToMe = (MyLoc2D - PoliceLoc2D).GetSafeNormal();

		FVector PoliceForward = Police->GetActorForwardVector();
		PoliceForward.Z = 0.f;
		PoliceForward.Normalize();

		if (FVector::DotProduct(PoliceForward, DirectionToMe) < CosHalfAngle) continue;

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Police);

		const bool bHit = World->LineTraceSingleByChannel(HitResult, PoliceLoc, MyLoc, ECC_Visibility, QueryParams);

		if (!bHit || HitResult.GetActor() == OwnerActor)
		{
			bAmIInFlashlight = true;
			break;
		}
	}

	if (bWasInFlashlight != bAmIInFlashlight)
	{
		bWasInFlashlight = bAmIInFlashlight;

		FHeistFlashlightAlertMessage Message;
		Message.bIsDetected = bAmIInFlashlight;

		UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(World);
		MessageSubsystem.BroadcastMessage(HeistMessageTags::Message_UI_FlashlightAlert, Message);
	}
}
