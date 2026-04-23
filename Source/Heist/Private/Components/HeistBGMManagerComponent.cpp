#include "Components/HeistBGMManagerComponent.h"

#include "Systems/Audio/HeistAudioSubsystem.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Components/FlashlightComponent.h"
#include "Character/PoliceCharacter.h"
#include "Character/ThiefCharacter.h"

#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UHeistBGMManagerComponent::UHeistBGMManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ActiveChaseCount = 0;
	CurrentMatchPhase = EHeistMatchPhase::None;
}

void UHeistBGMManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!IsValid(PC) || !PC->IsLocalController()) return;

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);

	PhaseChangeListenerHandle = MessageSubsystem.RegisterListener<FHeistPhaseChangedMessage>(
		HeistMessageTags::Message_Phase_Changed,
		[this](FGameplayTag Channel, const FHeistPhaseChangedMessage& Message)
		{
			OnPhaseChanged(Channel, Message);
		});
}

void UHeistBGMManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPawnBindings();

	if (PhaseChangeListenerHandle.IsValid())
	{
		PhaseChangeListenerHandle.Unregister();
	}

	Super::EndPlay(EndPlayReason);
}

void UHeistBGMManagerComponent::BindToPawn(APawn* InPawn)
{
	if (!IsValid(InPawn)) return;

	ClearPawnBindings();
	ActiveChaseCount = 0;

	if (AThiefCharacter* ThiefPawn = Cast<AThiefCharacter>(InPawn))
	{
		AlertMessageListenerHandle = UHeistMessageSubsystem::Get(this).RegisterListener<FHeistFlashlightAlertMessage>(
			HeistMessageTags::Message_UI_FlashlightAlert,
			[this](FGameplayTag Channel, const FHeistFlashlightAlertMessage& Message)
			{
				OnFlashlightAlertMessage(Channel, Message);
			});
	}
	else if (APoliceCharacter* PolicePawn = Cast<APoliceCharacter>(InPawn))
	{
		UFlashlightComponent* FlashlightComp = PolicePawn->GetFlashlightComponent();
		if (IsValid(FlashlightComp))
		{
			FlashlightComp->OnThiefSpotted.AddDynamic(this, &UHeistBGMManagerComponent::OnThiefSpottedByPolice);
			FlashlightComp->OnThiefLost.AddDynamic(this, &UHeistBGMManagerComponent::OnThiefLostByPolice);
		}
	}

	UpdateBGMState();
}

void UHeistBGMManagerComponent::ClearPawnBindings()
{
	if (AlertMessageListenerHandle.IsValid())
	{
		AlertMessageListenerHandle.Unregister();
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!IsValid(PC)) return;

	APoliceCharacter* PolicePawn = Cast<APoliceCharacter>(PC->GetPawn());
	if (!IsValid(PolicePawn)) return;

	UFlashlightComponent* FlashlightComp = PolicePawn->GetFlashlightComponent();
	if (!IsValid(FlashlightComp)) return;

	FlashlightComp->OnThiefSpotted.RemoveDynamic(this, &UHeistBGMManagerComponent::OnThiefSpottedByPolice);
	FlashlightComp->OnThiefLost.RemoveDynamic(this, &UHeistBGMManagerComponent::OnThiefLostByPolice);
}

void UHeistBGMManagerComponent::OnPhaseChanged(FGameplayTag Channel, const FHeistPhaseChangedMessage& Message)
{
	CurrentMatchPhase = Message.CurrentPhase;

	if (CurrentMatchPhase == EHeistMatchPhase::Result)
	{
		ActiveChaseCount = 0;
	}

	UpdateBGMState();
}

void UHeistBGMManagerComponent::OnFlashlightAlertMessage(FGameplayTag Channel, const FHeistFlashlightAlertMessage& Message)
{
	ActiveChaseCount = Message.bIsDetected ? 1 : 0;
	UpdateBGMState();
}

void UHeistBGMManagerComponent::OnThiefSpottedByPolice(AThiefCharacter* SpottedThief)
{
	ActiveChaseCount++;
	UpdateBGMState();
}

void UHeistBGMManagerComponent::OnThiefLostByPolice(AThiefCharacter* LostThief)
{
	ActiveChaseCount = FMath::Max(0, ActiveChaseCount - 1);

	UpdateBGMState();
}

void UHeistBGMManagerComponent::UpdateBGMState()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	UHeistAudioSubsystem* AudioSubsystem = World->GetSubsystem<UHeistAudioSubsystem>();
	if (!IsValid(AudioSubsystem)) return;

	if (CurrentMatchPhase == EHeistMatchPhase::Briefing || CurrentMatchPhase == EHeistMatchPhase::Execution)
	{
		if (ActiveChaseCount > 0)
		{
			AudioSubsystem->TransitionToBGM(EHeistSoundType::BGM_Game_Chase);
		}
		else
		{
			AudioSubsystem->TransitionToBGM(EHeistSoundType::BGM_Game_Normal);
		}
	}
	else if (CurrentMatchPhase == EHeistMatchPhase::Result)
	{
		AudioSubsystem->TransitionToBGM(EHeistSoundType::BGM_Result);
	}
}
