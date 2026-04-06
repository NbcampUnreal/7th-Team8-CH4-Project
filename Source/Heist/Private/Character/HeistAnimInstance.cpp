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
	IK_FootSpeedThreshold_Cached = IK_FootSpeedThreshold_Default;
}

// 기존 상태를 설정 시 동기화
void UHeistAnimInstance::SyncAllTagsEvents(UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC)) return;
	
	bIsSneaking = ASC->HasMatchingGameplayTag(HeistStateTags::State_Sneaking);
	// 새 상태 추가시 여기 수정
	bIsEscorted = ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Escorted);
	bIsCuffed = ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed);
	
	RefreshIKFootSpeedThresholdCached(); // IK 임계값 캐시 초기화
}

void UHeistAnimInstance::BindAllTagsEvents(UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC)) return;
	
	SneakingTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Sneaking,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistAnimInstance::HandleTagChanged);
	// 새 상태 추가시 여기 수정
	CuffedTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Cuffed,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistAnimInstance::HandleTagChanged);
	EscortedTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Escorted,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistAnimInstance::HandleTagChanged);
}

void UHeistAnimInstance::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bActive = (NewCount > 0);
	
	// 동시에 처리할 수 없는 상태일 경우 여기에 else if로 추가한다
	// 추후 이식성을 고려해 도둑과 경찰 모두 이곳에서 관리한다, CPP를 분리하지 않음
	if (Tag == HeistStateTags::State_Sneaking)
	{
		bIsSneaking = bActive;
	} else if (Tag == HeistStateTags::State_Thief_Escorted)
	{
		bIsEscorted = bActive;
	}
	
	if (Tag == HeistStateTags::State_Thief_Cuffed)
	{
		bIsCuffed = bActive;
	}
	
	RefreshIKFootSpeedThresholdCached();
}
#pragma endregion AbilitySystem

#pragma region AnimIK

void UHeistAnimInstance::RefreshIKFootSpeedThresholdCached()
{
	float NewThreshold = IK_FootSpeedThreshold_Default;
	
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		for (const TPair<FGameplayTag, float>& Pair : IK_FootSpeedThresholdMap)
		{
			if (ASC->HasMatchingGameplayTag(Pair.Key))
			{
				NewThreshold = Pair.Value;
				break;
			}
		}
	}
	
	IK_FootSpeedThreshold_Cached = NewThreshold;
}

const FName UHeistAnimInstance::FootL(TEXT("foot_l"));
const FName UHeistAnimInstance::FootR(TEXT("foot_r"));

void UHeistAnimInstance::UpdateIK(float DeltaSeconds)
{
    UCharacterMovementComponent* MovComp = PlayerChar->GetCharacterMovement();
    if (!IsValid(MovComp)) return;

    const float SafeDelta = FMath::Max(DeltaSeconds, KINDA_SMALL_NUMBER);

    if (MovComp->IsFalling())
    {
        IK_Offset_L.Z      = FMath::FInterpTo(IK_Offset_L.Z,      0.f, DeltaSeconds, IK_InterpSpeed);
        IK_Offset_R.Z      = FMath::FInterpTo(IK_Offset_R.Z,      0.f, DeltaSeconds, IK_InterpSpeed);
        IK_Offset_Pelvis.Z = FMath::FInterpTo(IK_Offset_Pelvis.Z, 0.f, DeltaSeconds, IK_InterpSpeed);
        IK_Alpha_L         = FMath::FInterpTo(IK_Alpha_L, 0.f, DeltaSeconds, IK_InterpSpeed);
        IK_Alpha_R         = FMath::FInterpTo(IK_Alpha_R, 0.f, DeltaSeconds, IK_InterpSpeed);
        IK_Rotation_L      = FMath::RInterpTo(IK_Rotation_L, FRotator::ZeroRotator, DeltaSeconds, IK_InterpSpeed);
        IK_Rotation_R      = FMath::RInterpTo(IK_Rotation_R, FRotator::ZeroRotator, DeltaSeconds, IK_InterpSpeed);
        bLocalFootCacheInitialized = false;
        return;
    }

    USkeletalMeshComponent* Mesh = PlayerChar->GetMesh();
    if (!IsValid(Mesh)) return;

    const FTransform MeshTransform = Mesh->GetComponentTransform();
    const FVector SocketL_Local = MeshTransform.InverseTransformPosition(Mesh->GetSocketLocation(FootL));
    const FVector SocketR_Local = MeshTransform.InverseTransformPosition(Mesh->GetSocketLocation(FootR));

    float LocalSpeedL = 0.f;
    float LocalSpeedR = 0.f;

    if (bLocalFootCacheInitialized)
    {
        LocalSpeedL = (SocketL_Local - PrevLocalL).Size() / SafeDelta;
        LocalSpeedR = (SocketR_Local - PrevLocalR).Size() / SafeDelta;
    }
    else
    {
        bLocalFootCacheInitialized = true;
    }

    PrevLocalL = SocketL_Local;
    PrevLocalR = SocketR_Local;

    DoFootTrace(FootL, IK_Offset_L, IK_Rotation_L, bIK_HitL, DeltaSeconds);
    DoFootTrace(FootR, IK_Offset_R, IK_Rotation_R, bIK_HitR, DeltaSeconds);

    const float Threshold = IK_FootSpeedThreshold_Cached;
    IK_Alpha_L = FMath::FInterpTo(IK_Alpha_L, (bIK_HitL && LocalSpeedL < Threshold) ? 1.f : 0.f, DeltaSeconds, IK_InterpSpeed);
    IK_Alpha_R = FMath::FInterpTo(IK_Alpha_R, (bIK_HitR && LocalSpeedR < Threshold) ? 1.f : 0.f, DeltaSeconds, IK_InterpSpeed);

    // 골반: 두 발 오프셋 중 낮은 값 기준, 항상 0 이하
    const float TargetPelvisZ = FMath::Clamp(
        FMath::Min(IK_Offset_L.Z, IK_Offset_R.Z),
        -IK_ThighDeadZone, 0.f);

    IK_Offset_Pelvis.Z = FMath::FInterpTo(IK_Offset_Pelvis.Z, TargetPelvisZ, SafeDelta, IK_InterpSpeed);
}

void UHeistAnimInstance::DoFootTrace(FName SocketName, FVector& OutOffset, FRotator& OutRotation, bool& OutHit, float DeltaSeconds)
{
    auto ResetToZero = [&]()
    {
        OutHit = false;
    	OutOffset.Z = 0.f;  // 보간 제거
    	OutRotation = FRotator::ZeroRotator;  // 보간 제거
    };

    if (!IsValid(PlayerChar)) { ResetToZero(); return; }

    USkeletalMeshComponent* Mesh = PlayerChar->GetMesh();
    if (!IsValid(Mesh)) { ResetToZero(); return; }
    if (!Mesh->DoesSocketExist(SocketName)) { ResetToZero(); return; }

    const FVector SocketLoc = Mesh->GetSocketLocation(SocketName);
    const float CapsuleHalfHeight = PlayerChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float CapsuleBottomZ = PlayerChar->GetActorLocation().Z - CapsuleHalfHeight;

    // 트레이스: X/Y는 소켓 기준, Z는 캡슐 바닥 기준으로 고정 (피드백 루프 방지)
    const FVector TraceStart = FVector(SocketLoc.X, SocketLoc.Y, CapsuleBottomZ + 20.f);
    const FVector TraceEnd   = FVector(SocketLoc.X, SocketLoc.Y, CapsuleBottomZ - IK_Trace_Dist);

    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit, TraceStart, TraceEnd, IK_TraceChannel, TraceParams);

    // DrawDebugLine(GetWorld(), TraceStart, bHit ? Hit.ImpactPoint : TraceEnd,
    //     bHit ? FColor::Green : FColor::Red, false, -1.f, 0, 2.f);
    // if (bHit)
    //     DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 3.f, 8, FColor::Green, false, -1.f);

    if (bHit && Hit.ImpactNormal.Z >= NormalThreshold)
    {
        OutHit = true;

        // Z 오프셋: 지면 높이 - 캡슐 바닥 (항상 양수 = 발이 올라가야 함)
        // 캡슐 바닥 기준이므로 피드백 루프 없음
        const float TargetZ = Hit.ImpactPoint.Z + FootHeight - CapsuleBottomZ;
        OutOffset.Z = FMath::FInterpTo(OutOffset.Z, TargetZ, DeltaSeconds, IK_InterpSpeed);

    	// 경사 법선에서 캐릭터 전방 기준 Pitch각도만 추출
    	const FVector CharForward = PlayerChar->GetActorForwardVector();

    	// 전방 경사 (Pitch)
    	const float PitchAngle = FMath::RadiansToDegrees(
			FMath::Atan2((Hit.ImpactNormal | CharForward), Hit.ImpactNormal.Z));

    	// Pitch만 적용, Yaw/Roll은 0
    	const FRotator TargetRotation = FRotator(0.f, 0.f, PitchAngle);
    	OutRotation = FMath::RInterpTo(OutRotation, TargetRotation, DeltaSeconds, IK_InterpSpeed);
    }
    else
    {
        ResetToZero();
    }
}

#pragma endregion AnimIK
