#include "Voice/HeistVoipTravelUtils.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystem.h"
#include "Interfaces/VoiceInterface.h"
#include "VoipListenerSynthComponent.h"
#include "Components/AudioComponent.h"

void HeistVoipTravelUtils::ShutdownVoiceForTravel(UWorld* World, bool bStopLocalVoice)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (OSS == nullptr) return;

	IOnlineVoicePtr VoiceInterface = OSS->GetVoiceInterface();
	if (!VoiceInterface.IsValid()) return;

	if (bStopLocalVoice)
	{
		VoiceInterface->StopNetworkedVoice(0);
	}

	if (!IsValid(World)) return;

	if (AGameStateBase* GS = World->GetGameState())
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (!IsValid(PS) || !PS->GetUniqueId().IsValid()) continue;

			const TSharedPtr<const FUniqueNetId> UniqueId = PS->GetUniqueId().GetUniqueNetId();
			if (!UniqueId.IsValid()) continue;

			VoiceInterface->UnregisterRemoteTalker(*UniqueId);
		}
	}
}

void HeistVoipTravelUtils::RestoreRemoteTalkers(UWorld* World, const APlayerController* LocalController)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (OSS == nullptr) return;

	IOnlineVoicePtr VoiceInterface = OSS->GetVoiceInterface();
	if (!VoiceInterface.IsValid() || !IsValid(World)) return;

	if (AGameStateBase* GS = World->GetGameState())
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (!IsValid(PS) || !PS->GetUniqueId().IsValid()) continue;

			if (IsValid(LocalController) && PS == LocalController->PlayerState) continue;

			const TSharedPtr<const FUniqueNetId> UniqueId = PS->GetUniqueId().GetUniqueNetId();
			if (!UniqueId.IsValid()) continue;

			VoiceInterface->RegisterRemoteTalker(*UniqueId);
		}
	}
}

void HeistVoipTravelUtils::UnregisterTransientVoipComps()
{
	UE_LOG(LogTemp, Log, TEXT("[VoipCleanup] Begin transient cleanup"));

	for (TObjectIterator<UVoipListenerSynthComponent> It; It; ++It)
	{
		UVoipListenerSynthComponent* Comp = *It;
		if (!IsValid(Comp)) continue;

		// Outer가 UPackage(Transient)인 것만 — 액터 소유 컴포넌트는 건드리지 않음
		if (!Comp->GetOuter() || !Comp->GetOuter()->IsA<UPackage>()) continue;

		if (UAudioComponent* AudioComponent = Comp->GetAudioComponent())
		{
			if (IsValid(AudioComponent))
			{
				AudioComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				AudioComponent->Stop();

				if (AudioComponent->IsRegistered())
				{
					AudioComponent->UnregisterComponent();
					UE_LOG(LogTemp, Log, TEXT("[VoipCleanup] AudioComponent unregistered: %s"),
						*AudioComponent->GetFullName());
				}
			}
		}

		if (Comp->IsRegistered())
		{
			Comp->UnregisterComponent();
			UE_LOG(LogTemp, Log, TEXT("[VoipCleanup] VoipListenerSynthComponent unregistered: %s"),
				*Comp->GetFullName());
		}
	}
}
