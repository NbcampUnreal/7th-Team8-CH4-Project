
#include "Components/ThiefEscortComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Character/HeistAnimInstance.h"
#include "Character/HeistCharacter.h"
#include "Character/HeistTags_State.h"
#include "Character/ThiefCharacter.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/HeistPawnExtensionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/HeistTags_InitState.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "CableComponent.h"
#include "Components/CapsuleComponent.h"

const FName UThiefEscortComponent::NAME_ActorFeatureName("ThiefEscort");

UThiefEscortComponent::UThiefEscortComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false);
	SetIsReplicatedByDefault(true);
}

void UThiefEscortComponent::BeginPlay()
{
	Super::BeginPlay();

	RegisterInitStateFeature();
	BindOnActorInitStateChanged(UHeistPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);
	CheckDefaultInitialization();
}

void UThiefEscortComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindVisualState();
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UThiefEscortComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThiefEscortComponent, EscortedBy);
	DOREPLIFETIME(UThiefEscortComponent, EscortingThief);
}

bool UThiefEscortComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	if (!CurrentState.IsValid() && DesiredState == HeistInitStateTags::InitState_GameplayReady)
	{
		return Manager->HasFeatureReachedInitState(
			GetOwner(),
			UHeistPawnExtensionComponent::NAME_ActorFeatureName,
			HeistInitStateTags::InitState_GameplayReady);
	}

	return false;
}

void UThiefEscortComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	if (DesiredState != HeistInitStateTags::InitState_GameplayReady) return;

	UHeistPawnExtensionComponent* PawnExtension = UHeistPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	if (!IsValid(PawnExtension)) return;

	BindVisualState(PawnExtension->GetAbilitySystemComponent());
	CacheVisualComponents();
	UpdateVisualState();
}

void UThiefEscortComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UHeistPawnExtensionComponent::NAME_ActorFeatureName)
	{
		CheckDefaultInitialization();
	}
}

void UThiefEscortComponent::CheckDefaultInitialization()
{
	TryToChangeInitState(HeistInitStateTags::InitState_GameplayReady);
}

bool UThiefEscortComponent::BeginEscort(AHeistCharacter* InPolice, TSubclassOf<UGameplayEffect> EscortedEffectClass,
	UAbilitySystemComponent* SourceASC)
{
	AActor* OwnerActor = GetOwner();
	AThiefCharacter* OwnerThief = Cast<AThiefCharacter>(OwnerActor);
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || !IsValid(OwnerThief) || !IsValid(InPolice))
	{
		// 서버 아니면 False
		return false;
	}
	
	UAbilitySystemComponent* TargetASC = OwnerThief->GetAbilitySystemComponent();
	if (!IsValid(TargetASC)) return false;
	
	// 이미 Escort 중이면 중복해서 시작을 방지 - (Escorting GA에서 1차로 걸러주긴 합니다. 방어코드용)
	if (IsValid(EscortedBy)) return false;

	UThiefEscortComponent* PoliceEscortComp = InPolice->FindComponentByClass<UThiefEscortComponent>();
	if (IsValid(PoliceEscortComp) && IsValid(PoliceEscortComp->GetEscortingThief()))
	{
		return false;
	}
	
	if (IsValid(EscortedEffectClass) && IsValid(SourceASC))
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EscortedEffectClass, 1.f, Context);
		if (Spec.IsValid() && Spec.Data.IsValid())
		{
			EscortedEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	if (IsValid(PoliceEscortComp))
	{
		PoliceEscortComp->SetEscortingThief(OwnerThief);
	}
	SetEscortedBy(InPolice);
	return true;
}

void UThiefEscortComponent::InterruptEscort(UAbilitySystemComponent* SourceASC, bool bConvertToCuffed)
{
	InterruptEscort(DefaultCuffedEffectClass, SourceASC, bConvertToCuffed);
}

void UThiefEscortComponent::InterruptEscort(TSubclassOf<UGameplayEffect> InCuffedEffectClass,
	UAbilitySystemComponent* SourceASC, bool bConvertToCuffed)
{
	AActor* OwnerActor = GetOwner();
	AThiefCharacter* OwnerThief = Cast<AThiefCharacter>(OwnerActor);
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || !IsValid(OwnerThief)) return;
	
	UAbilitySystemComponent* TargetASC = OwnerThief->GetAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		SetEscortedBy(nullptr);
		return;
	}
	
	// Escorted GE 제거
	if (EscortedEffectHandle.IsValid())
	{
		TargetASC->RemoveActiveGameplayEffect(EscortedEffectHandle);
		EscortedEffectHandle.Invalidate();
	}
	
	// 정책 분기 - bConvertedToCuff (Default : true)
	// 수갑 채운 상태로 풀려날지 아닐지 정한다
	if (bConvertToCuffed && InCuffedEffectClass && SourceASC)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(InCuffedEffectClass, 1.f, Context);
		if (Spec.IsValid() && Spec.Data.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	AHeistCharacter* PreviousPolice = EscortedBy;
	if (IsValid(PreviousPolice))
	{
		if (UThiefEscortComponent* PoliceEscortComp = PreviousPolice->FindComponentByClass<UThiefEscortComponent>())
		{
			PoliceEscortComp->SetEscortingThief(nullptr);
		}
	}

	SetEscortedBy(nullptr);
}

bool UThiefEscortComponent::IsEscortedBy(const AHeistCharacter* InPolice) const
{
	return IsValid(InPolice) && EscortedBy == InPolice;
}

AThiefCharacter* UThiefEscortComponent::FindEscortedThiefByPolice(const AHeistCharacter* InPolice)
{
	if (!IsValid(InPolice)) return nullptr;

	const UThiefEscortComponent* EscortComp = InPolice->FindComponentByClass<UThiefEscortComponent>();
	return IsValid(EscortComp) ? EscortComp->GetEscortingThief() : nullptr;
}

void UThiefEscortComponent::SetEscortedBy(AHeistCharacter* InPolice)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()) return;
	
	EscortedBy = InPolice;
	if (!InPolice)
	{
		PolicePositionTrail.Empty();
		MeshVisualRecoveryTimer = 0.0f;
	}
	
	ApplyEscortReplicationState();
}

void UThiefEscortComponent::SetEscortingThief(AThiefCharacter* InThief)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()) return;

	EscortingThief = InThief;
}

void UThiefEscortComponent::ApplyEscortReplicationState()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (OwnerActor->HasAuthority())
	{
		SetComponentTickEnabled(IsValid(EscortedBy));
		CacheRopeFromPolice(EscortedBy);
		UpdateVisualState();
		return;
	}

	// 클라도 physics vfx 복구는 필요하므로 Tick은 유지하되, 이동은 아래 authority 체크에서 막는다.
	SetComponentTickEnabled(IsValid(EscortedBy));
	CacheRopeFromPolice(EscortedBy);
	UpdateVisualState();
}

void UThiefEscortComponent::OnRep_EscortedBy()
{
	ApplyEscortReplicationState();
}

void UThiefEscortComponent::OnRep_EscortingThief()
{
}

void UThiefEscortComponent::BindVisualState(UHeistAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC)) return;
	if (CachedASC == ASC && bVisualStateReady) return;

	UnbindVisualState();

	CachedASC = ASC;
	CuffedTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Cuffed,
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UThiefEscortComponent::HandleVisualTagChanged);
	EscortedTagChangedHandle = ASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Escorted,
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UThiefEscortComponent::HandleVisualTagChanged);

	bCachedCuffed = ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed);
	bCachedEscorted = ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Escorted);
	bVisualStateReady = true;
}

void UThiefEscortComponent::UnbindVisualState()
{
	if (IsValid(CachedASC))
	{
		if (CuffedTagChangedHandle.IsValid())
		{
			CachedASC->RegisterGameplayTagEvent(
				HeistStateTags::State_Thief_Cuffed,
				EGameplayTagEventType::NewOrRemoved).Remove(CuffedTagChangedHandle);
		}

		if (EscortedTagChangedHandle.IsValid())
		{
			CachedASC->RegisterGameplayTagEvent(
				HeistStateTags::State_Thief_Escorted,
				EGameplayTagEventType::NewOrRemoved).Remove(EscortedTagChangedHandle);
		}
	}

	CuffedTagChangedHandle.Reset();
	EscortedTagChangedHandle.Reset();
	CachedASC = nullptr;
	bVisualStateReady = false;
	bCachedCuffed = false;
	bCachedEscorted = false;
}

void UThiefEscortComponent::CacheVisualComponents()
{
	CacheCuffComponent();
	CacheRopeFromPolice(EscortedBy);
}

void UThiefEscortComponent::CacheCuffComponent()
{
	CachedCuffComponent = FindNamedComponent<USkeletalMeshComponent>(GetOwner(), CuffComponentName);
}

void UThiefEscortComponent::CacheRopeFromPolice(AHeistCharacter* InPolice)
{
	if (IsValid(CachedRopeComponent) && (!IsValid(InPolice) || CachedRopeComponent->GetOwner() != InPolice))
	{
		SetComponentVisualHidden(CachedRopeComponent, true);
	}

	CachedRopeComponent = FindNamedComponent<UCableComponent>(InPolice, RopeComponentName);
}

void UThiefEscortComponent::UpdateVisualState()
{
	const bool bEscortRelationActive = IsValid(EscortedBy);
	const bool bShowCuff = bCachedCuffed || bCachedEscorted || bEscortRelationActive;
	const bool bShowRope = bEscortRelationActive;

	if (!IsValid(CachedCuffComponent))
	{
		CacheCuffComponent();
	}

	if (bShowRope && !IsValid(CachedRopeComponent))
	{
		CacheRopeFromPolice(EscortedBy);
	}

	if (IsValid(CachedCuffComponent))
	{
		SetComponentVisualHidden(CachedCuffComponent, !bShowCuff);
	}

	if (IsValid(CachedRopeComponent))
	{
		if (bShowRope)
		{
			ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
			if (IsValid(OwnerCharacter))
			{
				CachedRopeComponent->bAttachEnd = true;
				CachedRopeComponent->SetAttachEndTo(OwnerCharacter, TEXT("Mesh"), RopeEndSocketName);
			}
		}

		SetComponentVisualHidden(CachedRopeComponent, !bShowRope);
	}
}

void UThiefEscortComponent::SetComponentVisualHidden(USceneComponent* Component, bool bHidden) const
{
	if (!IsValid(Component)) return;
	
	Component->SetHiddenInGame(bHidden, true);
	Component->SetVisibility(!bHidden, true);
}

void UThiefEscortComponent::HandleVisualTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bActive = NewCount > 0;

	if (Tag == HeistStateTags::State_Thief_Cuffed)
	{
		bCachedCuffed = bActive;
	}
	else if (Tag == HeistStateTags::State_Thief_Escorted)
	{
		bCachedEscorted = bActive;
	}

	UpdateVisualState();
}

void UThiefEscortComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);
	if (!IsValid(OwnerCharacter)) return;

	// 메시가 캡슐 아래로 비정상 추락 시 스냅 복구
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	UHeistAnimInstance* AnimInst = IsValid(Mesh)
		? Cast<UHeistAnimInstance>(Mesh->GetAnimInstance())
		: nullptr;

	if (IsValid(EscortedBy) && IsValid(Mesh) && IsValid(AnimInst))
	{
		if (MeshVisualRecoveryTimer > 0.0f)
		{
			MeshVisualRecoveryTimer = FMath::Max(0.0f, MeshVisualRecoveryTimer - DeltaTime);
			if (MeshVisualRecoveryTimer <= 0.0f)
			{
				AnimInst->SetEscortPelvisPhysicsEnabled(true);
			}
		}
		else if (Mesh->IsAnySimulatingPhysics())
		{
			const float CapsuleBottomZ = OwnerCharacter->GetActorLocation().Z - 
						OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			const float MeshBottomZ = Mesh->Bounds.Origin.Z - Mesh->Bounds.BoxExtent.Z;
			const float MeshFallDistance = CapsuleBottomZ - MeshBottomZ;

			if (MeshFallDistance > MeshFallThreshold)
			{
				AnimInst->SetEscortPelvisPhysicsEnabled(false);
				MeshVisualRecoveryTimer = MeshVisualRecoveryDelay;
			}
		}
	}
	else
	{
		MeshVisualRecoveryTimer = 0.0f;
	}

	// 이동 처리는 서버가 관제한다
	if (!OwnerActor->HasAuthority()) return;
	if (!IsValid(EscortedBy)) return;

	// 거리 기반 트레일 기록 - 일정 거리 기준으로 떨어지면 기록해서 역추적
	const FVector PoliceLocation = EscortedBy->GetActorLocation();
	const FVector LastRecorded = PolicePositionTrail.Num() > 0 ? PolicePositionTrail.Last() : PoliceLocation;
	
	if (FVector::Dist(PoliceLocation, LastRecorded) >= TrailRecordInterval)
	{
		PolicePositionTrail.Add(PoliceLocation);
		if (PolicePositionTrail.Num() > MaxPositionTrailSize)
		{
			PolicePositionTrail.RemoveAt(0);
		}
	}
	
	const float DirectDist = FVector::Dist2D(PoliceLocation, OwnerActor->GetActorLocation());
	if (DirectDist > EscortOffsetDistance * 3.0f) // 기존 거리 Offeset 3배 초과시 , 이건 그냥 매직넘버로 둡니다. OD를 조정 가능하니
	{
		PolicePositionTrail.Empty();
	}
	
	// 트레일 안차있으면 경찰 위치를 바로 타겟 지정
	FVector TargetLocation = PoliceLocation - EscortedBy->GetActorForwardVector() * EscortOffsetDistance;
	if (PolicePositionTrail.Num() > 0)
	{
		for (int32 i = PolicePositionTrail.Num() - 1; i >= 0; --i)
		{
			if (FVector::Dist(PoliceLocation, PolicePositionTrail[i]) >= EscortOffsetDistance)
			{
				TargetLocation = PolicePositionTrail[i];
				break;
			}
		}
	}

	
	// 회전 보간
	const FRotator NewRotation = FMath::RInterpTo(
			OwnerActor->GetActorRotation(),
			EscortedBy->GetActorRotation(),
			DeltaTime,
			EscortRotationInterpSpeed);
	OwnerCharacter->SetActorRotation(NewRotation);
	
	// 이동 — bZOverride=false 로 중력 유지 (계단 대응)
	const FVector ToTarget = TargetLocation - OwnerActor->GetActorLocation();
	const float Dist = ToTarget.Size2D();

	if (Dist > LaunchIntervalDistance)
	{
		const float Speed = FMath::Clamp(Dist * EscortFollowInterpSpeed, 0.f, EscortMaxSpeed);
		OwnerCharacter->LaunchCharacter(ToTarget.GetSafeNormal() * Speed, true, false);
	}
	else
	{
		OwnerCharacter->LaunchCharacter(FVector::ZeroVector, true, false);
	}
}
