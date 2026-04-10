#include "Actors/ItemActor.h"

#include "Components/BoxComponent.h"
#include "AbilitySystem/HeistTags_FlagTags.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "Data/ItemData.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Character/HeistCharacter.h"
#include "GameplayTagContainer.h"
#include "Components/HeistInteractSphereComponent.h"

AItemActor::AItemActor() : bIsCarried(false), bIsSoloCarried(false)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; 
	SetReplicateMovement(true);

	//SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	//SetRootComponent(SceneRoot);

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	//BoxCollision->SetupAttachment(Mesh);
	SetRootComponent(BoxCollision);

	BoxCollision->SetSimulatePhysics(true);
	BoxCollision->SetCollisionProfileName(TEXT("PhysicsActor"));
	BoxCollision->SetUseCCD(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(BoxCollision);

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractSphereComponent = CreateDefaultSubobject<UHeistInteractSphereComponent>(TEXT("InteractSphereComponent"));
}

void AItemActor::Multicast_OnItemPhysicsEvent_Implementation(FVector ImpulseDir, float Force)
{
	BoxCollision->SetSimulatePhysics(true);
	BoxCollision->AddImpulse(ImpulseDir * Force, NAME_None, true);

    // 3. 서버에서만 착지 감지 및 타이머 시작
    if (HasAuthority())
    {
        GetWorldTimerManager().SetTimer(PhysicsTimeoutHandle, this, &AItemActor::FinalizePhysicsLocation, 2.0f, false);

        // 폭발물인 경우 낙하 시 폭발 체크
		if (const FItemData* Data = GetItemData())
		{
			if (Data->bExplosive)
			{ 
				OnExplode();
			}
		}
    }
}

void AItemActor::BeginPlay()
{
	Super::BeginPlay();
	InitializeFromData();

	InteractSphereComponent->OnCanInteract.BindUObject(this, &AItemActor::CheckCanInteract);
	InteractSphereComponent->OnGetAbilityTag.BindUObject(this, &AItemActor::ResolveInteractAbilityTag);
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

void AItemActor::UpdateCarryingState(const TArray<ACharacter*>& CurrentCarriers)
{
	// 기획에 따른 인원수 체크 및 Tag 부여 로직 (서버)
}

void AItemActor::FinalizePhysicsLocation()
{
	if (!HasAuthority()) return;

	// 물리 비활성화 및 현재 위치 확정
	Mesh->SetSimulatePhysics(false);

	// 현재 위치가 ReplicatedMovement를 통해 클라이언트로 전파됨
	FVector FinalLocation = GetActorLocation();
	SetActorLocation(FinalLocation);
}

void AItemActor::OnPickedUp(AHeistCharacter* InCarrier)
{
	if (!InCarrier || !HasAuthority()) return;
	bIsCarried = true;
}

int32 AItemActor::GetRequiredCarriers() const
{
	return GetItemData()->RequiredCarriers;
}

float AItemActor::GetCarrySpeedMultiplier() const
{
	return GetItemData()->CarrySpeedMultiplier;
}

float AItemActor::GetSoloCarrySpeedMultiplier() const
{
	return GetItemData()->SoloCarrySpeedMultiplier;
}

bool AItemActor::CheckCanInteract(ACharacter* Interactor) const
{
	if (bIsCarried && !bIsSoloCarried) return false;
	return true;
}

FGameplayTag AItemActor::ResolveInteractAbilityTag(ACharacter* Interactor) const
{
	return HeistEventTags::Event_CarryStarted;
}
