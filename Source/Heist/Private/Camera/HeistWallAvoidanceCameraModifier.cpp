#include "Camera/HeistWallAvoidanceCameraModifier.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

bool UHeistWallAvoidanceCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);

	AActor* ViewTarget = CameraOwner ? CameraOwner->GetViewTarget() : nullptr;
	if (!IsValid(ViewTarget)) return false;

	UWorld* World = ViewTarget->GetWorld();
	if (!IsValid(World)) return false;

	const FVector CharacterLocation = ViewTarget->GetActorLocation();
	const FVector IntendedCameraLocation = InOutPOV.Location;
	const float ArmLength = FVector::Dist(CharacterLocation, IntendedCameraLocation);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ViewTarget);

	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		CharacterLocation,
		IntendedCameraLocation,
		TraceChannel,
		QueryParams
	);

	if (!bHit) return false;

	// 벽이 캐릭터에 가까울수록 BlendWeight가 1에 가까워져 MaxPitch에 수렴한다.
	const float BlendWeight = FMath::Clamp(1.0f - (HitResult.Distance / ArmLength), 0.0f, 1.0f);
	const float NewPitch = FMath::Lerp(DefaultPitch, MaxPitch, BlendWeight);

	// 캐릭터와의 거리(ArmLength)를 유지한 채 피치만 바꿔 카메라를 위로 올린다.
	const FRotator NewRotation(NewPitch, InOutPOV.Rotation.Yaw, 0.0f);
	InOutPOV.Location = CharacterLocation - NewRotation.Vector() * ArmLength;
	InOutPOV.Rotation = NewRotation;

	return false;
}
