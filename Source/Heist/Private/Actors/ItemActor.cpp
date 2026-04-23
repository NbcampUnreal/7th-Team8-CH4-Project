#include "Actors/ItemActor.h"

#include "AbilitySystem/HeistTags_FlagTags.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "Components/HeistInteractSphereComponent.h"
#include "Components/HeistTransparencyComponent.h"
#include "Character/HeistCharacter.h"
#include "Data/ItemData.h"

#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"

AItemActor::AItemActor() : CurrentCarrierCount(0)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(true);

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	SetRootComponent(BoxCollision);

	BoxCollision->SetSimulatePhysics(true);
	BoxCollision->SetCollisionProfileName(TEXT("PhysicsActor"));
	BoxCollision->SetUseCCD(true);
	BoxCollision->ComponentTags.Add(FName("MainBody"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(BoxCollision);

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractSphereComponent = CreateDefaultSubobject<UHeistInteractSphereComponent>(TEXT("InteractSphereComponent"));

	TransparencyComponent = CreateDefaultSubobject<UHeistTransparencyComponent>(TEXT("TransparencyComponent"));
	TransparencyComponent->bStartInvisibleToPolice = false;
}

int32 AItemActor::GetItemValue()
{
	if (GetItemData()) return GetItemData()->Value;
	return 0;
}

void AItemActor::BeginPlay()
{
	Super::BeginPlay();
	InitializeFromData();

	InteractSphereComponent->OnCanInteract.BindUObject(this, &AItemActor::CheckCanInteract);
	InteractSphereComponent->OnGetAbilityTag.BindUObject(this, &AItemActor::ResolveInteractAbilityTag);
}

void AItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const TArray<FCarrierEntry>& ActiveCarriers = HasAuthority()
		? ReplicatedCarriers  // 서버는 자기 배열 사용
		: ReplicatedCarriers; // 클라이언트도 복제된 배열로 직접 계산

	if (ActiveCarriers.Num() > 0)
	{

		FVector SumLocation = FVector::ZeroVector;
		FQuat CombineQuat = FQuat::Identity;
		float SumZ = 0.f;
		bool bFirst = true;

		for (const FCarrierEntry& Entry : ActiveCarriers)
		{
			AHeistCharacter* Carrier = Entry.Carrier;
			if (!IsValid(Carrier)) continue;

			FVector CarrierTarget = Carrier->GetActorLocation()
				+ (Carrier->GetActorForwardVector() * CarryDistance);
			SumLocation += CarrierTarget;
			SumZ += Carrier->GetActorLocation().Z;

			FRotator SumRotation = Carrier->GetActorRotation() + Entry.StartRotator;
			if (bFirst) { CombineQuat = SumRotation.Quaternion(); bFirst = false; }
			else { CombineQuat += SumRotation.Quaternion(); }
		}

		FVector TargetLocation = SumLocation / ActiveCarriers.Num();
		TargetLocation.Z = SumZ / ActiveCarriers.Num();

		FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, 20.f);
		SetActorLocation(NewLocation, true);

		CombineQuat.Normalize();

		// 3. 회전도 부드럽게 따라가게 설정
		FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), CombineQuat.Rotator(), DeltaTime, 10.f);
		SetActorRotation(NewRotation);
	}

	if (HasAuthority()) CheckDrop();
}

void AItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemActor, CurrentCarrierCount);
	DOREPLIFETIME(AItemActor, ReplicatedCarriers);
}

void AItemActor::InitializeFromData()
{
	const FItemData* Data = GetItemData();
	if (!Data) return;

	// 기획: 데이터 테이블에 따라 초기 상태 설정
	// 예: GPS 발동 등급이면 서버에서 미리 마킹 준비 등
	if (HasAuthority() && Data->bTriggerGPS)
	{
		// GPS 관련 로직 (Event.GPSActivated 등)
	}
}

void AItemActor::OnExplode_Implementation()
{
	//TODO: 폭발 로직 구현
}

const FItemData* AItemActor::GetItemData() const
{
	if (ItemData.IsNull()) return nullptr;

	return ItemData.GetRow<FItemData>(TEXT("Context_ItemActor"));
}

void AItemActor::CheckDrop()
{
	// 서버에서만 연산하며, 들고 있는 사람이 있을 때만 실행
	if (!HasAuthority() || CurrentCarriers.Num() == 0) return;

	TArray<AHeistCharacter*> CarriersToDrop;

	for (TPair<AHeistCharacter*, FRotator> It : CurrentCarriers)
	{
		AHeistCharacter* Carrier = It.Key;
		if (!IsValid(Carrier)) continue;

		// 1. 위치 정보 가져오기
		FVector ItemLoc = GetActorLocation();
		FVector CarrierLoc = Carrier->GetActorLocation();

		// 2. 거리 계산
		float CurrentDistance = FVector::Dist(ItemLoc, CarrierLoc);

		// 3. 각도 계산 (기존 로직)
		FVector DirToItem = (ItemLoc - CarrierLoc).GetSafeNormal();
		FVector Forward = Carrier->GetActorForwardVector();
		float DotProduct = FVector::DotProduct(Forward, DirToItem);
		float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

		// --- 낙하 조건 체크 ---
		bool bShouldDrop = false;

		// 조건 1: 각도가 너무 큰 경우
		if (AngleDegrees > DropAngleMax) bShouldDrop = true;

		// 조건 2: 거리가 너무 멀어진 경우 (Max 초과)
		if (CurrentDistance > CarryDistanceMax) bShouldDrop = true;

		if (bShouldDrop)
		{
			CarriersToDrop.Add(Carrier);
		}
	}

	// 조건에 걸린 사람들에게 종료 이벤트 발송
	for (AHeistCharacter* TargetCarrier : CarriersToDrop)
	{
		DropCarrier(TargetCarrier);
	}
}

void AItemActor::OnPickedUp(AHeistCharacter* InCarrier)
{
	if (!IsValid(InCarrier) || !HasAuthority()) return;

	if (CurrentCarrierCount <= 0)
	{
		SetReplicateMovement(false);
		BoxCollision->SetSimulatePhysics(false);
		BoxCollision->SetEnableGravity(false);
		SetActorTickEnabled(true);
	}
	if (!CurrentCarriers.Contains(InCarrier))
	{
		FRotator RelativeRot = GetActorRotation() - InCarrier->GetActorRotation();
		CurrentCarriers.Add(InCarrier, RelativeRot);
		CurrentCarrierCount++;

		BoxCollision->IgnoreActorWhenMoving(InCarrier, true);

		FCarrierEntry Entry;
		Entry.Carrier = InCarrier;
		Entry.StartRotator = RelativeRot;
		ReplicatedCarriers.Add(Entry);

		OnRep_CurrentCarrierCount();
	}
	NotifyCarriersUpdate();
}

void AItemActor::OnDropOff(AHeistCharacter* InCarrier)
{
	if (!IsValid(InCarrier) || !HasAuthority()) return;

	if (CurrentCarriers.Contains(InCarrier))
	{
		CurrentCarriers.Remove(InCarrier);
		CurrentCarrierCount--;

		BoxCollision->IgnoreActorWhenMoving(InCarrier, false);

		ReplicatedCarriers.RemoveAll([InCarrier](const FCarrierEntry& E) {
			return E.Carrier == InCarrier;
			});

		OnRep_CurrentCarrierCount();
	}
	if (CurrentCarrierCount <= 0)
	{
		BoxCollision->SetEnableGravity(true);
		BoxCollision->SetSimulatePhysics(true);
		SetActorTickEnabled(false);
		SetReplicateMovement(true);
	}
	NotifyCarriersUpdate();
}

void AItemActor::OnRep_CurrentCarrierCount()
{
	if (!IsValid(TransparencyComponent)) return;

	const bool bShouldBeVisibleToPolice = (CurrentCarrierCount <= 0);
	TransparencyComponent->SetTargetVisibility(bShouldBeVisibleToPolice);
}

void AItemActor::OnRep_CarrierEntries()
{
	if (ReplicatedCarriers.Num() > 0)
	{
		BoxCollision->SetSimulatePhysics(false);
		BoxCollision->SetEnableGravity(false);
		SetActorTickEnabled(true);

		for (const FCarrierEntry& Entry : ReplicatedCarriers)
		{
			if (IsValid(Entry.Carrier))
			{
				BoxCollision->IgnoreActorWhenMoving(Entry.Carrier, true);
			}
		}
	}
	else
	{
		BoxCollision->SetEnableGravity(true);
		BoxCollision->SetSimulatePhysics(true);
		SetActorTickEnabled(false);

		BoxCollision->ClearMoveIgnoreActors();
	}
}

int32 AItemActor::GetRequiredCarriers_Implementation() const
{
	return GetItemData()->RequiredCarriers;
}

float AItemActor::GetCarrySpeedMultiplier_Implementation(int32 CarrierCount) const
{
	if (CurrentCarrierCount == 1) return GetItemData()->SoloCarrySpeedMultiplier;
	else if (CurrentCarrierCount >= 2) return GetItemData()->CarrySpeedMultiplier;

	return 1.f;
}

bool AItemActor::CheckCanInteract(ACharacter* Interactor) const
{
	if (GetItemData()->RequiredCarriers <= CurrentCarrierCount) return false;
	return true;
}

FGameplayTag AItemActor::ResolveInteractAbilityTag(ACharacter* Interactor) const
{
	return HeistEventTags::Event_CarryStarted;
}

void AItemActor::NotifyCarriersUpdate()
{
	FGameplayTag UpdateTag = FGameplayTag::RequestGameplayTag(TEXT("Event.CarryUpdate"));
	FGameplayEventData Payload;
	Payload.Target = this;

	for (TPair<AHeistCharacter*, FRotator> It : CurrentCarriers)
	{
		AHeistCharacter* Carrier = It.Key;
		if (IsValid(Carrier) && Carrier->GetAbilitySystemComponent())
		{
			// 모든 캐리어의 ASC에 이벤트를 전송하여 어빌리티의 OnCarryUpdateEventReceived를 트리거함
			Carrier->GetAbilitySystemComponent()->HandleGameplayEvent(UpdateTag, &Payload);
		}
	}
}

void AItemActor::DropCarrier(AHeistCharacter* Carrier)
{
	if (Carrier && Carrier->GetAbilitySystemComponent())
	{
		FGameplayTag EndTag = FGameplayTag::RequestGameplayTag(TEXT("Event.CarryDrop"));
		FGameplayEventData Payload;

		Carrier->GetAbilitySystemComponent()->HandleGameplayEvent(EndTag, &Payload);
	}
}
