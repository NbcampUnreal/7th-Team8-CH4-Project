
#include "Components/HeistInteractSphereComponent.h"

#include "Components/HeistInteractionComponent.h"
#include "Components/SphereComponent.h"

UHeistInteractSphereComponent::UHeistInteractSphereComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHeistInteractSphereComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// DT 에서 반경 읽기
	if (InteractDataTable && !DataTableRowName.IsNone())
	{
		if (const FHeistInteractData* InteractData = InteractDataTable->FindRow<FHeistInteractData>(DataTableRowName, TEXT("")))
		{
			CachedRadius = InteractData->InteractRadius;
		}
	}
	
	// SphereComponent를 동적 생성합니다
	InteractSphere = NewObject<USphereComponent>(GetOwner(), TEXT("InteractSphere"));
	InteractSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	
	// 1. 실제로 스피어의 반지름을 데이터 테이블 값으로 설정 (누락된 부분 추가)
	InteractSphere->SetSphereRadius(CachedRadius);
	
	// 2. 게임 내에서 스피어 콜리전이 보이도록 설정 (디버그용)
	InteractSphere->SetHiddenInGame(false);
	
	InteractSphere->RegisterComponent();
	InteractSphere->AttachToComponent(GetOwner()->GetRootComponent(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &UHeistInteractSphereComponent::OnSphereBeginOverlap);
	InteractSphere->OnComponentEndOverlap.AddDynamic(this, &UHeistInteractSphereComponent::OnSphereEndOverlap);

	// BeginOverlap를 놓친 초기 겹침분을 보정한다.
	TArray<AActor*> OverlappingActors;
	InteractSphere->GetOverlappingActors(OverlappingActors);

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (UHeistInteractionComponent* IC = OverlappingActor->FindComponentByClass<UHeistInteractionComponent>())
		{
			IC->RegisterInteractable(TScriptInterface<IHeistInteractable>(this));
		}
	}
}

bool UHeistInteractSphereComponent::CanInteract_Implementation(ACharacter* Interactor) const
{
	// C++ 델레게이트 바인딩 확인
	if (OnCanInteract.IsBound())
		return OnCanInteract.Execute(Interactor);
	
	// 없으면 BP에서 구현한거 실행 - 선택해서 사용할 수 있어요
	AActor* Owner = GetOwner();
	if (IsValid(Owner) && Owner->GetClass()->ImplementsInterface(UHeistInteractable::StaticClass()))
		return IHeistInteractable::Execute_CanInteract(Owner, Interactor);
	
	return false;
}

FGameplayTag UHeistInteractSphereComponent::GetInteractAbilityTag_Implementation(ACharacter* Interactor) const
{
	if (OnGetAbilityTag.IsBound()) return OnGetAbilityTag.Execute(Interactor);
	
	AActor* Owner = GetOwner();

	if (IsValid(Owner) && Owner->GetClass()->ImplementsInterface(UHeistInteractable::StaticClass()))
		return IHeistInteractable::Execute_GetInteractAbilityTag(Owner, Interactor);
	
	return FGameplayTag::EmptyTag;
}

float UHeistInteractSphereComponent::GetInteractRadius_Implementation() const
{
	return CachedRadius;
}

// Overlap 시 처리
void UHeistInteractSphereComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// ScriptInterface 객체로 감싸면 RawPointer로 안보내고 좀더 엔진 네이티브하게 처리할 수 있나 봅니다
	if (UHeistInteractionComponent* IC = OtherActor->FindComponentByClass<UHeistInteractionComponent>())
		IC->RegisterInteractable(TScriptInterface<IHeistInteractable>(this));
}

void UHeistInteractSphereComponent::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (UHeistInteractionComponent* IC = OtherActor->FindComponentByClass<UHeistInteractionComponent>())
		IC->UnregisterInteractable(TScriptInterface<IHeistInteractable>(this));
}
