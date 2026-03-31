#include "Components/HeistPawnExtensionComponent.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Data/HeistPawnData.h"
#include "Data/HeistTags_InitState.h"

UHeistPawnExtensionComponent::UHeistPawnExtensionComponent()
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
	CheckDefaultInitialization();
}

void UHeistPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

bool UHeistPawnExtensionComponent::HasReachedInitState(const FGameplayTag& State) const
{
	// 순서: DataAvailable < DataInitialized < GameplayReady
	if (State == HeistInitStateTags::InitState_DataAvailable)
	{
		return CurrentInitState == HeistInitStateTags::InitState_DataAvailable
			|| CurrentInitState == HeistInitStateTags::InitState_DataInitialized
			|| CurrentInitState == HeistInitStateTags::InitState_GameplayReady;
	}

	if (State == HeistInitStateTags::InitState_DataInitialized)
	{
		return CurrentInitState == HeistInitStateTags::InitState_DataInitialized
			|| CurrentInitState == HeistInitStateTags::InitState_GameplayReady;
	}

	if (State == HeistInitStateTags::InitState_GameplayReady)
		return CurrentInitState == HeistInitStateTags::InitState_GameplayReady;

	return false;
}

void UHeistPawnExtensionComponent::CheckDefaultInitialization()
{
	if (!HasReachedInitState(HeistInitStateTags::InitState_DataAvailable)
		&& CanTransitionToState(HeistInitStateTags::InitState_DataAvailable))
	{
		HandleInitStateTransition(HeistInitStateTags::InitState_DataAvailable);
	}

	if (!HasReachedInitState(HeistInitStateTags::InitState_DataInitialized)
		&& CanTransitionToState(HeistInitStateTags::InitState_DataInitialized))
	{
		HandleInitStateTransition(HeistInitStateTags::InitState_DataInitialized);
	}

	if (!HasReachedInitState(HeistInitStateTags::InitState_GameplayReady)
		&& CanTransitionToState(HeistInitStateTags::InitState_GameplayReady))
	{
		HandleInitStateTransition(HeistInitStateTags::InitState_GameplayReady);
	}
}

bool UHeistPawnExtensionComponent::CanTransitionToState(const FGameplayTag& NewState) const
{
	if (NewState == HeistInitStateTags::InitState_DataAvailable)
		return IsValid(PawnData);

	if (NewState == HeistInitStateTags::InitState_DataInitialized)
		return HasReachedInitState(HeistInitStateTags::InitState_DataAvailable) && IsValid(AbilitySystemComponent);

	if (NewState == HeistInitStateTags::InitState_GameplayReady)
		return HasReachedInitState(HeistInitStateTags::InitState_DataInitialized);

	return false;
}

void UHeistPawnExtensionComponent::HandleInitStateTransition(const FGameplayTag& NewState)
{
	CurrentInitState = NewState;

	if (NewState == HeistInitStateTags::InitState_GameplayReady)
	{
		OnGameplayReady.Broadcast();
	}
}
