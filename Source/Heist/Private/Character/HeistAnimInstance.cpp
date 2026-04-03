#include "Character/HeistAnimInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/HeistTags_State.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UHeistAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	TryBindAbilitySystem();
	
	PlayerChar = Cast<ACharacter>(GetOwningActor());
	if (!IsValid(PlayerChar)) return;

	TraceParams = FCollisionQueryParams(FName("IK_FootTrace"),false, PlayerChar);
}

void UHeistAnimInstance::NativeUninitializeAnimation()
{
	UnbindAbilitySystem();
	Super::NativeUninitializeAnimation();
}

void UHeistAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	// ASC 지연 생성 더블체크
	if (!CachedASC.IsValid() || !bTagBindingReady)
	{
		TryBindAbilitySystem();
	}
	
	PlayerChar = Cast<ACharacter>(GetOwningActor());
	if (!IsValid(PlayerChar)) return;
	
	UpdateIK(DeltaSeconds);
}

#pragma region AbilitySystem

void UHeistAnimInstance::TryBindAbilitySystem()
{
	APawn* OwnerPawn = TryGetPawnOwner();
	if (!IsValid(OwnerPawn)) return;
	
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerPawn);
	if (!IsValid(ASC)) return;
	
	if (CachedASC.Get() == ASC) return;
	
	UnbindAbilitySystem();
	
	CachedASC = ASC;
	SyncAllTagsEvents(ASC); // 상태 등록
	BindAllTagsEvents(ASC); // 이벤트 구독
	bTagBindingReady = true;
}

void UHeistAnimInstance::UnbindAbilitySystem()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	
	if (IsValid(ASC))
	{
		if (SneakingTagChangedHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(HeistStateTags::State_Sneaking, EGameplayTagEventType::NewOrRemoved).Remove(SneakingTagChangedHandle);
		}
		// 새 상태 추가시 여기 수정
	}
	
	SneakingTagChangedHandle.Reset();
	CachedASC.Reset();
	bTagBindingReady = false;
}

// 기존 상태를 설정 시 동기화
void UHeistAnimInstance::SyncAllTagsEvents(UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC)) return;
	
	bIsSneaking = ASC->HasMatchingGameplayTag(HeistStateTags::State_Sneaking);
	
	// 새 상태 추가시 여기 수정
}

void UHeistAnimInstance::BindAllTagsEvents(UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC)) return;
	
	SneakingTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Sneaking,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistAnimInstance::HandleTagChanged);
	
	// 새 상태 추가시 여기 수정
}

void UHeistAnimInstance::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bActive = (NewCount > 0);

	if (Tag == HeistStateTags::State_Sneaking)
	{
		bIsSneaking = bActive;
	}
}
#pragma endregion AbilitySystem

#pragma region AnimIK

const FName UHeistAnimInstance::FootL(TEXT("foot_l"));
const FName UHeistAnimInstance::FootR(TEXT("foot_r"));

void UHeistAnimInstance::UpdateIK(float DeltaSeconds)
{
	// GetCharacterMovement null 방어
	UCharacterMovementComponent* MovComp = PlayerChar->GetCharacterMovement();
	if (!IsValid(MovComp)) return;
	
	if (MovComp->IsFalling())
	{
		// 공중에서는 IK 보간 복귀
		IK_Offset_L      = FMath::VInterpTo(IK_Offset_L,      FVector::ZeroVector,   DeltaSeconds, IK_InterpSpeed);
		IK_Offset_R      = FMath::VInterpTo(IK_Offset_R,      FVector::ZeroVector,   DeltaSeconds, IK_InterpSpeed);
		IK_Offset_Pelvis = FMath::VInterpTo(IK_Offset_Pelvis, FVector::ZeroVector,   DeltaSeconds, IK_InterpSpeed);
		IK_Rotation_L    = FMath::RInterpTo(IK_Rotation_L,    FRotator::ZeroRotator, DeltaSeconds, IK_InterpSpeed);
		IK_Rotation_R    = FMath::RInterpTo(IK_Rotation_R,    FRotator::ZeroRotator, DeltaSeconds, IK_InterpSpeed);
	
		// 공중에서는 Alpha도 0으로 복귀
		IK_Alpha_L = FMath::FInterpTo(IK_Alpha_L, 0.f, DeltaSeconds, IK_InterpSpeed);
		IK_Alpha_R = FMath::FInterpTo(IK_Alpha_R, 0.f, DeltaSeconds, IK_InterpSpeed);

		return;
	}
	
	DoFootTrace(FootL, IK_Offset_L, IK_Rotation_L, DeltaSeconds);
	DoFootTrace(FootR, IK_Offset_R, IK_Rotation_R, DeltaSeconds);

	// float TargetPelvisZ = FMath::Clamp(FMath::Min(IK_Offset_L.Z, IK_Offset_R.Z),-60.f, 20.f);
	//float TargetPelvisZ = FMath::Max(IK_Offset_L.Z, IK_Offset_R.Z);
	
	// IK_Offset_Pelvis.Z = FMath::FInterpTo(
	// 	IK_Offset_Pelvis.Z,
	// 	TargetPelvisZ,
	// 	DeltaSeconds,
	// 	IK_InterpSpeed
	// );
	
	// IK_Offset_Pelvis.Z = FMath::FInterpTo(
	// 	IK_Offset_Pelvis.Z,
	// 	TargetPelvisZ,
	// 	DeltaSeconds,
	// 	IK_InterpSpeed
	// );

	// 오프셋 크기에 따라 Alpha 결정 — 평지(≈0)에서는 IK가 애니메이션을 덮지 않음
	const float TargetAlphaL = FMath::Clamp(FMath::Abs(IK_Offset_L.Z) / IK_AlphaBlendThreshold, 0.f, 1.f);
	const float TargetAlphaR = FMath::Clamp(FMath::Abs(IK_Offset_R.Z) / IK_AlphaBlendThreshold, 0.f, 1.f);
	IK_Alpha_L = FMath::FInterpTo(IK_Alpha_L, TargetAlphaL, DeltaSeconds, IK_InterpSpeed);
	IK_Alpha_R = FMath::FInterpTo(IK_Alpha_R, TargetAlphaR, DeltaSeconds, IK_InterpSpeed);

	float TargetPelvisZ = FMath::Clamp(FMath::Min(IK_Offset_L.Z, IK_Offset_R.Z), -50.f, 0.f);
	IK_Offset_Pelvis.Z = FMath::FInterpTo(IK_Offset_Pelvis.Z, TargetPelvisZ, DeltaSeconds, IK_InterpSpeed);

}

void UHeistAnimInstance::DoFootTrace(FName SocketName, FVector& OutOffset, FRotator& OutRotation, float DeltaSeconds)
{
	// 실패 경로도 항상 보간 복귀
	auto ResetToZero = [&]()
	{
		OutOffset.Z = FMath::FInterpTo(OutOffset.Z, 0.f, DeltaSeconds, IK_InterpSpeed);
		OutRotation = FMath::RInterpTo(OutRotation, FRotator::ZeroRotator, DeltaSeconds, IK_InterpSpeed);
	};
	
	if (!IsValid(PlayerChar))
	{
		ResetToZero();
		return;
	}
	
	USkeletalMeshComponent* Mesh = PlayerChar->GetMesh();
	if (!IsValid(Mesh))
	{
		ResetToZero();
		return;
	}
	
	if (!Mesh->DoesSocketExist(SocketName))
	{
		ResetToZero(); 
		return;
	}
	
	FVector SocketLoc = Mesh->GetSocketLocation(SocketName);

	FTransform ActorTransform = PlayerChar->GetActorTransform();
	FVector LocalSocketOffset = ActorTransform.InverseTransformPosition(SocketLoc);
	LocalSocketOffset.Z = 0.f;

	float CapsuleHalfHeight = PlayerChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float CapsuleBottomZ    = PlayerChar->GetActorLocation().Z - CapsuleHalfHeight;

	FVector TraceOrigin = ActorTransform.TransformPosition(FVector(LocalSocketOffset.X, LocalSocketOffset.Y, 0.f));
	TraceOrigin.Z = CapsuleBottomZ;

	FVector TraceStart = TraceOrigin + FVector(0, 0, IK_Trace_Dist);
	FVector TraceEnd   = TraceOrigin - FVector(0, 0, IK_Trace_Dist);

	
	// // 소켓의 로컬 오프셋을 매 프레임 액터 트랜스폼 기준으로 계산
	// // (IK가 본을 움직여도 로컬 오프셋은 고정)
	// FTransform ActorTransform = PlayerChar->GetActorTransform();
	// FVector LocalSocketOffset = ActorTransform.InverseTransformPosition(SocketLoc);
	// LocalSocketOffset.Z = 0.f; // Z는 무시, XY만 사용
	//
	// // 월드 위치 재계산: 액터 기준 XY + 캡슐 바닥 Z
	// float CapsuleHalfHeight = PlayerChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	// float CapsuleBottomZ    = PlayerChar->GetActorLocation().Z - CapsuleHalfHeight;
	//
	// FVector TraceOrigin = ActorTransform.TransformPosition(FVector(LocalSocketOffset.X, LocalSocketOffset.Y, 0.f));
	// TraceOrigin.Z = CapsuleBottomZ;
	//
	// FVector TraceStart = TraceOrigin + FVector(0, 0, IK_Trace_Dist);
	// // DoFootTrace 내 TraceStart 계산 직후 추가
	// UE_LOG(LogTemp, Warning, TEXT("[%s] TraceStart Z: %f, CapsuleBottomZ: %f, SocketLoc Z: %f"),
	// 	*SocketName.ToString(),
	// 	TraceStart.Z,
	// 	CapsuleBottomZ,
	// 	SocketLoc.Z
	// );
	// FVector TraceEnd   = TraceOrigin - FVector(0, 0, IK_Trace_Dist);
	
	FHitResult Hit;
	// bool bHit = GetWorld()->LineTraceSingleByChannel(
	// 	Hit,
	// 	SocketLoc + FVector(0, 0, IK_Trace_Dist),
	// 	SocketLoc - FVector(0, 0, IK_Trace_Dist),
	// 	IK_TraceChannel,
	// 	TraceParams
	// );
	
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		IK_TraceChannel,
		TraceParams
	);
	
	// // 디버그 라인
	if (bHit)
	{
		DrawDebugLine(GetWorld(), TraceStart, Hit.ImpactPoint, FColor::Green, false, -1.f, 0, 2.f);
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.f, 8, FColor::Green, false, -1.f);
	}
	else
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, -1.f, 0, 2.f);
	}

	if (bHit)
	{
		// → 골반이 내려가 발 소켓이 낮아지면 TargetZ > 0 (무릎 꺾어서 위로 올림)
		const float TargetZ = Hit.ImpactPoint.Z - SocketLoc.Z + FootHeight;
		OutOffset.Z = FMath::FInterpTo(OutOffset.Z, TargetZ, DeltaSeconds, IK_InterpSpeed);

		const FRotator TargetRot = FRotationMatrix::MakeFromZX(
			Hit.ImpactNormal,
			PlayerChar->GetActorForwardVector()
		).Rotator();
		OutRotation = FMath::RInterpTo(OutRotation, TargetRot, DeltaSeconds, IK_InterpSpeed);

		//float TargetZ = Hit.ImpactPoint.Z - FootLoc.Z + FootHeight;
//		float TargetZ = -(Hit.ImpactPoint.Z - SocketLoc.Z + FootHeight);
		// const float TargetZ = Hit.ImpactPoint.Z - TraceOrigin.Z;
		// OutOffset.Z = FMath::FInterpTo(OutOffset.Z, TargetZ, DeltaSeconds, IK_InterpSpeed);
		//
		// const FRotator TargetRot = FRotationMatrix::MakeFromZX(
		// 	Hit.ImpactNormal,
		// 	PlayerChar->GetActorForwardVector()
		// ).Rotator();
		// OutRotation = FMath::RInterpTo(OutRotation, TargetRot, DeltaSeconds, IK_InterpSpeed);
		
		// float TargetZ = Hit.ImpactPoint.Z - SocketLoc.Z + FootHeight;
		//
		// OutOffset.Z = FMath::FInterpTo(OutOffset.Z, TargetZ, DeltaSeconds, IK_InterpSpeed);
		//
		// // FRotator TargetRot = FRotationMatrix::MakeFromZX(
		// //  Hit.ImpactNormal,
		// //  PlayerChar->GetActorForwardVector()
		// // ).Rotator();
		//
		// FRotator TargetRot = FRotationMatrix::MakeFromYX(
		// 	Hit.ImpactNormal,
		// 	PlayerChar->GetActorForwardVector()
		// ).Rotator();
		// OutRotation = FMath::RInterpTo(OutRotation, TargetRot, DeltaSeconds, IK_InterpSpeed);
	}
	else
	{
		ResetToZero();
	}
}

#pragma endregion AnimIK
