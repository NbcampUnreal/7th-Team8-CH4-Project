#include "Components/HeistHitReactionComponent.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Components/HeistPawnExtensionComponent.h"
#include "Data/HeistTags_InitState.h"
#include "Systems/Audio/HeistAudioSubsystem.h"
#include "Character/PoliceCharacter.h"

#include "GameFramework/Actor.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/World.h"

const FName UHeistHitReactionComponent::NAME_ActorFeatureName("HitReaction");

UHeistHitReactionComponent::UHeistHitReactionComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

UHeistHitReactionComponent* UHeistHitReactionComponent::FindHitReactionComponent(const AActor* Actor)
{
	if (!IsValid(Actor)) return nullptr;
	return Actor->FindComponentByClass<UHeistHitReactionComponent>();
}

void UHeistHitReactionComponent::SetMeleeHitHandler(const FHeistMeleeHitDelegate& InHandler)
{
	MeleeHitHandler = InHandler;
}

void UHeistHitReactionComponent::ResetMeleeHitHandler()
{
	MeleeHitHandler.Unbind();
}

bool UHeistHitReactionComponent::HasMeleeHitHandler() const
{
	return MeleeHitHandler.IsBound();
}

void UHeistHitReactionComponent::ProcessMeleeHit(AActor* InstigatorActor, AActor* TargetActor) const
{
	if (!MeleeHitHandler.IsBound()) return;
	if (!IsValid(InstigatorActor) || !IsValid(TargetActor)) return;

	FGameplayEventData Payload;
	Payload.Instigator = InstigatorActor;
	Payload.Target = TargetActor;
	MeleeHitHandler.Execute(Payload);
}

void UHeistHitReactionComponent::Multicast_PlayHitReaction_Implementation(bool bIsHitValid, AActor* InstigatorActor, const FVector& ImpactLocation)
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
	if (!IsValid(AudioSubsystem)) return;

	bool bIsPolice = false;
	if (IsValid(InstigatorActor))
	{
		bIsPolice = InstigatorActor->IsA<APoliceCharacter>();
	}

	AudioSubsystem->PlayOneShotSound(bIsPolice ? EHeistSoundType::Swing : EHeistSoundType::Kick, ImpactLocation);

	if (bIsHitValid)
	{
		AudioSubsystem->PlayOneShotSound(bIsPolice ? EHeistSoundType::Hit_Police : EHeistSoundType::Hit_Thief, ImpactLocation);
		// TODO (하민): 피격 파티클 스폰이나 카메라 셰이크 연동 시 여기에 추가
	}
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
