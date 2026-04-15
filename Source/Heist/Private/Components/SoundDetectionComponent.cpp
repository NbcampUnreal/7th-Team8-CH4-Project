#include "Components/SoundDetectionComponent.h"

#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "AbilitySystem/HeistTags_Event.h"

#include "GameFramework/Pawn.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTargetTypes.h"

USoundDetectionComponent::USoundDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USoundDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerPawn);
	if (!IsValid(ASC))
	{
		// TODO(하민): PlayerState에 ASC가 존재하는 경우 BeginPlay 시점에 ASC가 없을 수 있습니다.
		// 이 경우 OnPossessed 혹은 ASC 초기화 완료 델리게이트를 통해 바인딩하도록 수정이 필요합니다.
		return;
	}

	ASC->GenericGameplayEventCallbacks.FindOrAdd(HeistEventTags::Event_SoundDetected).AddUObject(this, &ThisClass::OnSoundDetectedEvent);
}

void USoundDetectionComponent::OnSoundDetectedEvent(const FGameplayEventData* Payload)
{
	if (Payload == nullptr) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn)) return;

	// 로컬 컨트롤러(실제 화면을 보고 있는 플레이어)에게만 UI를 띄워야 합니다.
	if (!OwnerPawn->IsLocallyControlled()) return;

	FVector SoundLocation = FVector::ZeroVector;

	if (Payload->TargetData.Num() > 0)
	{
		const FGameplayAbilityTargetData* TargetData = Payload->TargetData.Get(0);
		if (TargetData != nullptr)
		{
			SoundLocation = TargetData->GetEndPointTransform().GetLocation();
		}
	}

	FHeistSoundDetectedMessage Message;
	Message.OriginLocation = SoundLocation;
	Message.DetectionRadius = Payload->EventMagnitude;

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(GetWorld());
	MessageSubsystem.BroadcastMessage(HeistMessageTags::Message_UI_SoundDetected, Message);
}
