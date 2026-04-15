#include "Character/HeistAnimInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/HeistTags_State.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
	ClearRestraintHandGoal();
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
	
	UpdateRestraintHandGoal();
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
		if (StunnedTagChangedHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(HeistStateTags::State_Stunned, EGameplayTagEventType::NewOrRemoved).Remove(StunnedTagChangedHandle);
		}
		if (KnockbackTagChangedHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(HeistStateTags::State_Knockback, EGameplayTagEventType::NewOrRemoved).Remove(KnockbackTagChangedHandle);
		}
		if (CuffedTagChangedHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(HeistStateTags::State_Thief_Cuffed, EGameplayTagEventType::NewOrRemoved).Remove(CuffedTagChangedHandle);
		}
		if (EscortedTagChangedHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(HeistStateTags::State_Thief_Escorted, EGameplayTagEventType::NewOrRemoved).Remove(EscortedTagChangedHandle);
		}
		if (InjuredTagChangedHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(HeistStateTags::State_Thief_Injured, EGameplayTagEventType::NewOrRemoved).Remove(InjuredTagChangedHandle);
		}
	}
	
	SneakingTagChangedHandle.Reset();
	StunnedTagChangedHandle.Reset();
	KnockbackTagChangedHandle.Reset();
	CuffedTagChangedHandle.Reset();
	EscortedTagChangedHandle.Reset();
	InjuredTagChangedHandle.Reset();
	CachedASC.Reset();
	bTagBindingReady = false;
	IK_FootSpeedThreshold_Cached = IK_FootSpeedThreshold_Default;
}

// 기존 상태를 설정 시 동기화
void UHeistAnimInstance::SyncAllTagsEvents(UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC)) return;
	
	bIsSneaking = ASC->HasMatchingGameplayTag(HeistStateTags::State_Sneaking);
	bIsStunned = ASC->HasMatchingGameplayTag(HeistStateTags::State_Stunned);
	bIsKnockbacked = ASC->HasMatchingGameplayTag(HeistStateTags::State_Knockback);
	bIsEscorted = ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Escorted);
	bIsCuffed = ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed);
	bIsInjured = ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured);
	
	ApplyRestraintPresentationState();
	RefreshIKFootSpeedThresholdCached(); // IK 임계값 캐시 초기화
}

void UHeistAnimInstance::BindAllTagsEvents(UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC)) return;
	
	SneakingTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Sneaking,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistAnimInstance::HandleTagChanged);
	StunnedTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Stunned,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistAnimInstance::HandleTagChanged);
	KnockbackTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Knockback,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistAnimInstance::HandleTagChanged);
	CuffedTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Cuffed,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistAnimInstance::HandleTagChanged);
	EscortedTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Escorted,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistAnimInstance::HandleTagChanged);
	InjuredTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Injured,
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
	} else if (Tag == HeistStateTags::State_Stunned)
	{
		bIsStunned = bActive;
	} else if (Tag == HeistStateTags::State_Knockback)
	{
		bIsKnockbacked = bActive;
	} else if (Tag == HeistStateTags::State_Thief_Escorted)
	{
		bIsEscorted = bActive;
		ApplyRestraintPresentationState();
	} else if (Tag == HeistStateTags::State_Thief_Injured)
	{
		bIsInjured = bActive;
	}
	
	if (Tag == HeistStateTags::State_Thief_Cuffed)
	{
		bIsCuffed = bActive;
		ApplyRestraintPresentationState();
	}
}

// 구속 소켓을 캐릭터 mesh component space로 변환해 AnimGraph가 바로 쓸 수 있는 goal을 만든다.
void UHeistAnimInstance::UpdateRestraintHandGoal()
{
	if (!bUseRestrainedHandIK || !bAutoResolveRightHandGoal)
	{
		ClearRestraintHandGoal();
		return;
	}
	
	if (!IsValid(PlayerChar))
	{
		ClearRestraintHandGoal();
		return;
	}
	
	USkeletalMeshComponent* CharacterMesh = PlayerChar->GetMesh();
	if (!IsValid(CharacterMesh))
	{
		ClearRestraintHandGoal();
		return;
	}
	
	USkeletalMeshComponent* GoalSource = ResolveRestraintGoalSource();
	if (!IsValid(GoalSource))
	{
		ClearRestraintHandGoal();
		return;
	}
	
	const FTransform GoalSocket_SourceCS = GoalSource->GetSocketTransform(RightHandGoalSocketName, RTS_Component);
	const FTransform GoalSourceToCharacterCS =
		GoalSource->GetComponentTransform().GetRelativeTransform(CharacterMesh->GetComponentTransform());

	RightHandGoal_CS = GoalSourceToCharacterCS.TransformPosition(GoalSocket_SourceCS.GetLocation());
	RightHandGoalRot_CS =
		GoalSourceToCharacterCS.TransformRotation(GoalSocket_SourceCS.GetRotation()).Rotator();
	const FVector CurrentWrist_CS =
		CharacterMesh->GetSocketTransform(TEXT("wrist_r"), RTS_Component).GetLocation();
	RightHandGoalOffset_CS = RightHandGoal_CS - CurrentWrist_CS;
	RightHandGoalAlpha = 1.f;
	bHasRightHandGoal = true;
}

// 상태가 끝난 뒤 이전 프레임 goal이 남아 있지 않도록 즉시 초기화한다.
void UHeistAnimInstance::ClearRestraintHandGoal()
{
	bHasRightHandGoal = false;
	RightHandGoalAlpha = 0.f;
	RightHandGoal_CS = FVector::ZeroVector;
	RightHandGoalOffset_CS = FVector::ZeroVector;
	RightHandGoalRot_CS = FRotator::ZeroRotator;
}

// 구속 상태 진입 시 발 IK 캐시를 즉시 비운다.
void UHeistAnimInstance::ResetFootIKImmediate()
{
	IK_Offset_L = FVector::ZeroVector;
	IK_Offset_R = FVector::ZeroVector;
	IK_Offset_Pelvis = FVector::ZeroVector;
	IK_Rotation_L = FRotator::ZeroRotator;
	IK_Rotation_R = FRotator::ZeroRotator;
	IK_Alpha_L = 0.f;
	IK_Alpha_R = 0.f;
	bIK_HitL = false;
	bIK_HitR = false;
	bLocalFootCacheInitialized = false;
}

void UHeistAnimInstance::SetEscortPelvisPhysicsEnabled(bool bEnabled)
{
	if (!bUseEscortPelvisPhysics)
	{
		bEnabled = false;
	}

	USkeletalMeshComponent* Mesh = IsValid(PlayerChar) ? PlayerChar->GetMesh() : nullptr;
	if (!IsValid(Mesh) || EscortPhysicsRootBone.IsNone()) return;

	const FName PhysicsRootBone = EscortPhysicsRootBone;
	const FName LeftArmBone = EscortPhysicsLeftArmBone;
	const FName RightArmBone = EscortPhysicsRightArmBone;
	const FName ChestBone = EscortPhysicsChestBone;
	const FName LeftUpperLegBone = EscortPhysicsLeftUpperLegBone;
	const FName RightUpperLegBone = EscortPhysicsRightUpperLegBone;
	const FName LeftFootBone = EscortPhysicsLeftFootBone;
	const FName RightFootBone = EscortPhysicsRightFootBone;

	auto EnablePhysicsBelow = [Mesh](const FName BoneName, float BlendWeight)
	{
		if (BoneName.IsNone()) return;

		Mesh->SetAllBodiesBelowSimulatePhysics(BoneName, true, true);
		Mesh->SetAllBodiesBelowPhysicsBlendWeight(BoneName, BlendWeight, false, true);
	};

	auto DisablePhysicsBelow = [Mesh](const FName BoneName, float BlendWeight)
	{
		if (BoneName.IsNone()) return;
		
		Mesh->SetAllBodiesBelowSimulatePhysics(BoneName, false, true);
		Mesh->SetAllBodiesBelowPhysicsBlendWeight(BoneName, BlendWeight, false, true);
	};

	if (!bEnabled)
	{
		DisablePhysicsBelow(PhysicsRootBone, 0.f);
		Mesh->ResetAllBodiesSimulatePhysics();
		return;
	}
	
	// hips부터 상체를 simulate 상태로 전환한다.
	EnablePhysicsBelow(PhysicsRootBone, EscortHipsBlendWeight);

	// 다리는 끌리는 실루엣을 유지하도록 별도 강도로 다시 활성화한다.
	EnablePhysicsBelow(LeftUpperLegBone, EscortLegBlendWeight);
	EnablePhysicsBelow(RightUpperLegBone, EscortLegBlendWeight);
	EnablePhysicsBelow(LeftFootBone, EscortFootBlendWeight);
	EnablePhysicsBelow(RightFootBone, EscortFootBlendWeight);
	
	// 수갑 포즈를 유지해야 하는 상체 chain은 blend를 0으로 잠근다.
	EnablePhysicsBelow(ChestBone, EscortLockedLimbBlendWeight);
	EnablePhysicsBelow(LeftArmBone, EscortLockedLimbBlendWeight);
	EnablePhysicsBelow(RightArmBone, EscortLockedLimbBlendWeight);
}

/**
 * Character에 미리 붙어 있는 수갑 SkeletalMeshComponent를 찾는다.
 * RightHandTarget의 실제 소유자여야 한다.
 */
USkeletalMeshComponent* UHeistAnimInstance::ResolveRestraintGoalSource() const
{ 
	if (!IsValid(PlayerChar)) return nullptr;
	
	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	PlayerChar->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
	
	for (USkeletalMeshComponent* MeshComp : SkeletalMeshComponents)
	{
		if (!IsValid(MeshComp)) continue;

		if (!MeshComp->ComponentHasTag(RestraintGoalSourceComponentTag))
		{
			continue;
		}

		if (!MeshComp->DoesSocketExist(RightHandGoalSocketName))
		{
			continue;
		}

		return MeshComp;
	}
	
	return nullptr;
}

// StateMachine이 고른 포즈 위에 구속 상태용 procedural 연출 스위치를 얹는다.
void UHeistAnimInstance::ApplyRestraintPresentationState()
{
	bIsRestrained = (bIsCuffed || bIsEscorted);
	bUseRestrainedHandIK =
		bIsRestrained && !(bDisableRestrainedHandIKWhileEscorted && bIsEscorted);

	RightHandGoalAlpha = bUseRestrainedHandIK ? 1.f : 0.f;
	
	if (!bIsRestrained)
	{
		ClearRestraintHandGoal();
	}
	
	if (bDisableFootIKWhenRestrained && bIsRestrained)
	{
		ResetFootIKImmediate();
	}

	SetEscortPelvisPhysicsEnabled(bIsEscorted);

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
	if (bDisableFootIKWhenRestrained && bIsRestrained)
	{
		ResetFootIKImmediate();
		return;
	}

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
