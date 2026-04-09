
#include "Components/HeistHitReactionComponent.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Character/HeistTags_State.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/HeistPawnExtensionComponent.h"
#include "Data/HeistTags_InitState.h"
#include "GameFramework/Character.h"

const FName UHeistHitReactionComponent::NAME_ActorFeatureName("HitReaction"); 

UHeistHitReactionComponent::UHeistHitReactionComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

UHeistHitReactionComponent* UHeistHitReactionComponent::FindHitReactionComponent(const AActor* Actor)
{
	if (!IsValid(Actor)) return nullptr;
	return Actor->FindComponentByClass<UHeistHitReactionComponent>();
}

void UHeistHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();
	RegisterInitStateFeature();
	
	// HeistPawnExtensionComponent 구독 — 변화 시 OnActorInitStateChanged 호출
	BindOnActorInitStateChanged(UHeistPawnExtensionComponent::NAME_ActorFeatureName,
	FGameplayTag(), false);

	CheckDefaultInitialization();
}

void UHeistHitReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

bool UHeistHitReactionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	// GameplayReady가 되었을 때만 True하여 상태 전이 허용
	if (!CurrentState.IsValid() && DesiredState == HeistInitStateTags::InitState_GameplayReady)
	{
		return Manager->HasFeatureReachedInitState(GetOwner(),
			UHeistPawnExtensionComponent::NAME_ActorFeatureName,
			HeistInitStateTags::InitState_GameplayReady);
	}
	return false;
}

void UHeistHitReactionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState != HeistInitStateTags::InitState_GameplayReady) return;

	UHeistPawnExtensionComponent* PawnExtension =
		UHeistPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	if (!IsValid(PawnExtension)) return;

	// PawnExtension에서 GAS 가져옴
	UHeistAbilitySystemComponent* ASC = PawnExtension->GetAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	// Stunned GameplayTagEvent를 바인딩
	ASC->RegisterGameplayTagEvent(HeistStateTags::State_Stunned, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UHeistHitReactionComponent::OnStunnedTagChanged);
	
	// 이외에도 상태 변화가 필요한 이벤트를 아래에 추가하세요
}

void UHeistHitReactionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UHeistPawnExtensionComponent::NAME_ActorFeatureName)
		CheckDefaultInitialization();
}

void UHeistHitReactionComponent::CheckDefaultInitialization()
{
	TryToChangeInitState(HeistInitStateTags::InitState_GameplayReady);
}

void UHeistHitReactionComponent::OnStunnedTagChanged(const FGameplayTag Tag, int32 Count)
{
	// Stunned 태그가 활성화되면 캐릭터를 스턴 상태로 만들고, 비활성화되면 스턴에서 해제하는 로직을 여기에 구현하세요.

	// 아래와 같이 일반화합니다
	if (Count > 0) return; // 스턴 진입은 GA의 PlayAnimMontage가 처리

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!IsValid(OwnerChar)) return;

	UAnimInstance* AnimInst = OwnerChar->GetMesh()->GetAnimInstance();
	if (IsValid(AnimInst) && IsValid(AnimInst->GetCurrentActiveMontage()))
		AnimInst->Montage_JumpToSection(FName("Outro"), AnimInst->GetCurrentActiveMontage());

}
