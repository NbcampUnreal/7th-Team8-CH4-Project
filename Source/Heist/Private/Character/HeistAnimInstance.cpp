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
	
	if (!IsValid(PlayerChar))
	{
		PlayerChar = Cast<ACharacter>(GetOwningActor());
		if (!IsValid(PlayerChar)) return;
	}
	
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
    UCharacterMovementComponent* MovComp = PlayerChar->GetCharacterMovement();
    if (!IsValid(MovComp)) return;

    if (MovComp->IsFalling())
    {
        IK_Offset_L      = FMath::VInterpTo(IK_Offset_L,      FVector::ZeroVector, DeltaSeconds, IK_InterpSpeed);
        IK_Offset_R      = FMath::VInterpTo(IK_Offset_R,      FVector::ZeroVector, DeltaSeconds, IK_InterpSpeed);
        IK_Offset_Pelvis = FMath::VInterpTo(IK_Offset_Pelvis, FVector::ZeroVector, DeltaSeconds, IK_InterpSpeed);
        IK_Alpha_L = FMath::FInterpTo(IK_Alpha_L, 0.f, DeltaSeconds, IK_InterpSpeed);
        IK_Alpha_R = FMath::FInterpTo(IK_Alpha_R, 0.f, DeltaSeconds, IK_InterpSpeed);
        return;
    }

    USkeletalMeshComponent* Mesh = PlayerChar->GetMesh();
    if (!IsValid(Mesh)) return;

    // 소켓 속도 계산
    FVector SocketL = Mesh->GetSocketLocation(FootL);
    FVector SocketR = Mesh->GetSocketLocation(FootR);

    float SpeedL = (SocketL - PrevSocketL).Size() / DeltaSeconds;
    float SpeedR = (SocketR - PrevSocketR).Size() / DeltaSeconds;

    PrevSocketL = SocketL;
    PrevSocketR = SocketR;

    DoFootTrace(FootL, IK_Offset_L, IK_Rotation_L, bIK_HitL, DeltaSeconds);
    DoFootTrace(FootR, IK_Offset_R, IK_Rotation_R, bIK_HitR, DeltaSeconds);

    float TargetAlphaL = (bIK_HitL && SpeedL < IK_FootSpeedThreshold) ? 1.f : 0.f;
    float TargetAlphaR = (bIK_HitR && SpeedR < IK_FootSpeedThreshold) ? 1.f : 0.f;

    IK_Alpha_L = FMath::FInterpTo(IK_Alpha_L, TargetAlphaL, DeltaSeconds, IK_InterpSpeed);
    IK_Alpha_R = FMath::FInterpTo(IK_Alpha_R, TargetAlphaR, DeltaSeconds, IK_InterpSpeed);

    float CapsuleBottomZ = PlayerChar->GetActorLocation().Z
                         - PlayerChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

    float TargetPelvisZ = FMath::Clamp(
        FMath::Min(IK_Offset_L.Z, IK_Offset_R.Z) - CapsuleBottomZ,
        -IK_ThighDeadZone, 0.f);
    IK_Offset_Pelvis.Z = FMath::FInterpTo(
        IK_Offset_Pelvis.Z, TargetPelvisZ, DeltaSeconds, IK_InterpSpeed);
}

void UHeistAnimInstance::DoFootTrace(FName SocketName, FVector& OutOffset, FRotator& OutRotation, bool& OutHit, float DeltaSeconds)
{
	auto ResetToZero = [&]()
	{
		OutHit = false;
		OutOffset.Z = FMath::FInterpTo(OutOffset.Z, 0.f, DeltaSeconds, IK_InterpSpeed);
		OutRotation = FMath::RInterpTo(OutRotation, FRotator::ZeroRotator, DeltaSeconds, IK_InterpSpeed);
	};
    
	if (!IsValid(PlayerChar)) { ResetToZero(); return; }
    
	USkeletalMeshComponent* Mesh = PlayerChar->GetMesh();
	if (!IsValid(Mesh)) { ResetToZero(); return; }
	if (!Mesh->DoesSocketExist(SocketName)) { ResetToZero(); return; }
    
	FVector SocketLoc         = Mesh->GetSocketLocation(SocketName);
	float   CapsuleHalfHeight = PlayerChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float   CapsuleBottomZ    = PlayerChar->GetActorLocation().Z - CapsuleHalfHeight;

	FVector TraceStart = FVector(SocketLoc.X, SocketLoc.Y, CapsuleBottomZ + 20.f);
	FVector TraceEnd   = FVector(SocketLoc.X, SocketLoc.Y, CapsuleBottomZ - IK_Trace_Dist);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, TraceStart, TraceEnd, IK_TraceChannel, TraceParams);

	DrawDebugLine(GetWorld(), TraceStart, bHit ? Hit.ImpactPoint : TraceEnd,
		bHit ? FColor::Green : FColor::Red, false, -1.f, 0, 2.f);
	if (bHit)
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 3.f, 8, FColor::Green, false, -1.f);

	if (bHit)
	{
		if (bHit && Hit.ImpactNormal.Z >= 0.5f)
		{
			OutHit = true;
			FVector TargetPos = FVector(SocketLoc.X, SocketLoc.Y, Hit.ImpactPoint.Z + FootHeight);
			OutOffset = FMath::VInterpTo(OutOffset, TargetPos, DeltaSeconds, IK_InterpSpeed);
			
			FQuat FromUpToNormal = FQuat::FindBetweenNormals(FVector::YAxisVector, Hit.ImpactNormal);
			OutRotation = FMath::RInterpTo(OutRotation, FromUpToNormal.Rotator(), DeltaSeconds, IK_InterpSpeed);
		}
	}
	else
	{
		ResetToZero();
	}
}

#pragma endregion AnimIK
