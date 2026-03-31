#include "Components/HeistPawnExtensionComponent.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "AbilitySystem/HeistAbilitySet.h"
#include "Data/HeistPawnData.h"
#include "Data/HeistTags_InitState.h"

#include "Components/GameFrameworkComponentManager.h"

const FName UHeistPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

UHeistPawnExtensionComponent::UHeistPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

UHeistPawnExtensionComponent* UHeistPawnExtensionComponent::FindPawnExtensionComponent(const AActor* Actor)
{
	if (!IsValid(Actor)) return nullptr;
	return Actor->FindComponentByClass<UHeistPawnExtensionComponent>();
}

void UHeistPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();
	RegisterInitStateFeature();
	CheckDefaultInitialization();
}

void UHeistPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	UninitializeAbilitySystem();
	Super::EndPlay(EndPlayReason);
}

void UHeistPawnExtensionComponent::SetPawnData(const UHeistPawnData* InPawnData)
{
	if (PawnData == InPawnData) return;
	PawnData = InPawnData;
	CheckDefaultInitialization();
}

void UHeistPawnExtensionComponent::InitializeAbilitySystem(UHeistAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
	if (AbilitySystemComponent == InASC) return;

	AbilitySystemComponent = InASC;
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, GetOwner());

	CheckDefaultInitialization();
}

void UHeistPawnExtensionComponent::UninitializeAbilitySystem()
{
	AbilitySystemComponent = nullptr;
}

bool UHeistPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	if (!CurrentState.IsValid() && DesiredState == HeistInitStateTags::InitState_DataAvailable)
		return IsValid(PawnData);

	if (CurrentState == HeistInitStateTags::InitState_DataAvailable
		&& DesiredState == HeistInitStateTags::InitState_DataInitialized)
		return IsValid(AbilitySystemComponent);

	if (CurrentState == HeistInitStateTags::InitState_DataInitialized
		&& DesiredState == HeistInitStateTags::InitState_GameplayReady)
		return true;

	return false;
}

void UHeistPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState == HeistInitStateTags::InitState_GameplayReady)
	{
		if (IsValid(PawnData) && IsValid(PawnData->DefaultAbilitySet))
		{
			PawnData->DefaultAbilitySet->GiveToAbilitySystem(AbilitySystemComponent);
		}
	}
}

void UHeistPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	// PawnExtension은 다른 피처의 상태 변화에 반응할 필요 없음
}

void UHeistPawnExtensionComponent::CheckDefaultInitialization()
{
	TryToChangeInitState(HeistInitStateTags::InitState_DataAvailable);
	TryToChangeInitState(HeistInitStateTags::InitState_DataInitialized);
	TryToChangeInitState(HeistInitStateTags::InitState_GameplayReady);
}
