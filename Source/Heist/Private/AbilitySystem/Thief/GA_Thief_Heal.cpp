#include "AbilitySystem/Thief/GA_Thief_Heal.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "Character/HeistTags_State.h"
#include "Character/ThiefCharacter.h"
#include "Components/HeistNoiseComponent.h"
#include "Systems/Audio/HeistAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"

UGA_Thief_Heal::UGA_Thief_Heal()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnGameplayEvent;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	AbilityTags.AddTag(HeistAbilityTags::Ability_Thief_Heal);
	//CancelAbilitiesWithTag.AddTag(HeistStateTags::State_Stunned);

	// 무력화 상태일 경우 진입 차단함
	//ActivationBlockedTags.AddTag(HeistStateTags::State_ActionDisabled);

	// Trigger Data를 세팅해주는 것 만으로 이 어빌리티 시스템은 GAS에 등록된 이상
	// GameplayEvent에 의해 Actiavated 될 수 있습니다. 와우
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = HeistAbilityTags::Ability_Thief_Heal;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Thief_Heal::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	// 트리거 데이터로 인해 켜졌을 경우? -> 트리거 데이터에서 타겟 액터를 찾아서 그 ASC를 캐싱해둬야겠죠
	const AActor* TargetActor = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;
	const AThiefCharacter* Thief = Cast<AThiefCharacter>(TargetActor);
	if (!IsValid(Thief))
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	UAbilitySystemComponent* ASC = Thief->GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}
	TargetASC = ASC;

	// FChannelingData의 RowName과 맞출 것
	// StartChanneling이 GE_Channeling을 적용하여 State.ActionDisabled / State.Channeling 부여
	StartChanneling(FName("Heal"));

	if (HasAuthority(&CurrentActivationInfo))
	{
		UHeistNoiseComponent* NoiseComponent = Thief->GetHeistNoiseComponent();
		if (IsValid(NoiseComponent))
		{
			NoiseComponent->StartChannelingNoise(EHeistSoundType::Heal);
		}
	}

	UWorld* World = GetWorld();
	if (IsValid(World) && World->GetNetMode() != NM_DedicatedServer)
	{
		UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
		if (IsValid(AudioSubsystem))
		{
			HealAudioComp = AudioSubsystem->PlayLoopingSound(EHeistSoundType::Heal, Thief->GetRootComponent());
		}
	}
}

void UGA_Thief_Heal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HasAuthority(&CurrentActivationInfo) && TargetASC.IsValid())
	{
		AThiefCharacter* TargetThief = Cast<AThiefCharacter>(TargetASC->GetAvatarActor());
		if (IsValid(TargetThief))
		{
			UHeistNoiseComponent* NoiseComponent = TargetThief->GetHeistNoiseComponent();
			if (IsValid(NoiseComponent))
			{
				NoiseComponent->StopChannelingNoise();
			}
		}
	}

	UWorld* World = GetWorld();
	if (IsValid(World) && World->GetNetMode() != NM_DedicatedServer)
	{
		UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
		if (IsValid(AudioSubsystem) && IsValid(HealAudioComp))
		{
			AudioSubsystem->StopLoopingSound(HealAudioComp);
			HealAudioComp = nullptr;
		}
	}

	TargetASC = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Thief_Heal::OnChannelingCompleted()
{
	if (!TargetASC.IsValid()) return;
	if (!HasAuthority(&CurrentActivationInfo)) return;

	// 수갑 GE 탐색 후 제거 로직
	FGameplayEffectQuery InjuredQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
		FGameplayTagContainer(HeistStateTags::State_Thief_Injured));
	TargetASC->RemoveActiveEffects(InjuredQuery);
}

