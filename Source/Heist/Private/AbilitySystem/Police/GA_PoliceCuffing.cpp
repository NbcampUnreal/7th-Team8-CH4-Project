#include "AbilitySystem/Police/GA_PoliceCuffing.h"

#include "Character/ThiefCharacter.h"
#include "Character/HeistTags_State.h"
#include "AbilitySystem/HeistTags_Ability.h"
#include "AbilitySystem/HeistTags_Event.h"
#include "Systems/Audio/HeistAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "Heist/Heist.h"

UGA_PoliceCuffing::UGA_PoliceCuffing()
{
	ActivationPolicy = EHeistAbilityActivationPolicy::OnGameplayEvent;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	AbilityTags.AddTag(HeistAbilityTags::Ability_Police_Cuffing);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = HeistAbilityTags::Ability_Police_Cuffing;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_PoliceCuffing::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) return;

	TargetThief = nullptr;

	// 상호작용 처리
	AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
	TargetThief = Cast<AThiefCharacter>(TargetActor);

	if (!IsValid(TargetThief))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartChanneling(FName("Cuffing"));

	UWorld* World = GetWorld();
	if (IsValid(World) && World->GetNetMode() != NM_DedicatedServer)
	{
		UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
		if (IsValid(AudioSubsystem))
		{
			CuffingAudioComp = AudioSubsystem->PlayLoopingSound(EHeistSoundType::Cuffing, TargetThief->GetRootComponent());
		}
	}
}

void UGA_PoliceCuffing::OnChannelingCompleted()
{
	if (HasAuthority(&CurrentActivationInfo) && IsValid(TargetThief))
	{
		UAbilitySystemComponent* TargetASC = TargetThief->GetAbilitySystemComponent();
		if (IsValid(TargetASC))
		{
			FGameplayEventData Payload;
			Payload.Instigator = GetAvatarActorFromActorInfo();
			Payload.Target = TargetThief;

			TargetASC->HandleGameplayEvent(HeistEventTags::Event_CuffingComplete, &Payload);

			// Injured GE 제거 후 Cuffed GE 적용
			FGameplayEffectQuery InjuredQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
				FGameplayTagContainer(HeistStateTags::State_Thief_Injured));
			TargetASC->RemoveActiveEffects(InjuredQuery);

			if (IsValid(CuffedEffectClass))
			{
				FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(CuffedEffectClass, 1.f);
				TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	UWorld* World = GetWorld();
	if (IsValid(World) && World->GetNetMode() != NM_DedicatedServer)
	{
		UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
		if (IsValid(AudioSubsystem) && IsValid(CuffingAudioComp))
		{
			AudioSubsystem->StopLoopingSound(CuffingAudioComp);
			CuffingAudioComp = nullptr;
		}
	}

	// NOTE:
	// 채널링 종료는 UHeistGameplayAbility의 공통 흐름(타이머 만료 -> Outro/몽타주 종료 -> EndAbility)에 맡긴다.
	// 여기서 직접 EndAbility(..., bReplicateEndAbility=true)를 호출하면
	// LocalPredicted 클라이언트 인스턴스가 서버 authoritative 인스턴스보다 먼저 종료를 복제할 수 있고,
	// 그 결과 서버가 Cuff 적용 완료 전에 종료되어 client 경찰에서 Cuffing이 실패할 수 있다.
	// EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PoliceCuffing::OnChannelingCancelled()
{
	UWorld* World = GetWorld();
	if (IsValid(World) && World->GetNetMode() != NM_DedicatedServer)
	{
		UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
		if (IsValid(AudioSubsystem) && IsValid(CuffingAudioComp))
		{
			AudioSubsystem->StopLoopingSound(CuffingAudioComp);
			CuffingAudioComp = nullptr;
		}
	}

	TargetThief = nullptr;
}
