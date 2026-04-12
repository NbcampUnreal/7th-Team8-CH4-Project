#include "Core/HeistPlayerController.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Character/HeistTags_State.h"
#include "Core/HeistPlayerState.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "OnlineSubsystem.h"
#include "Interfaces/VoiceInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"

AHeistPlayerController::AHeistPlayerController()
{
	bShowMouseCursor = true;
}

void AHeistPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsValid(SystemMenuInputAction)) return;

	UEnhancedInputComponent* EnhancedIC = CastChecked<UEnhancedInputComponent>(InputComponent);
	EnhancedIC->BindAction(SystemMenuInputAction, ETriggerEvent::Started, this, &ThisClass::Input_SystemMenu);
}

void AHeistPlayerController::Input_SystemMenu()
{
	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(HeistMessageTags::Message_SystemMenu_Toggle, FHeistSystemMenuToggleMessage{});
}

void AHeistPlayerController::AcknowledgePossession(APawn* NewPawn)
{
	Super::AcknowledgePossession(NewPawn);

	if (!IsLocalController()) return;

	StartTalking();

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (OSS == nullptr) return;

	IOnlineVoicePtr VoiceInterface = OSS->GetVoiceInterface();
	if (!VoiceInterface.IsValid()) return;

	if (VoiceTalkingStateChangedHandle.IsValid())
	{
		VoiceInterface->ClearOnPlayerTalkingStateChangedDelegate_Handle(VoiceTalkingStateChangedHandle);
	}

	VoiceTalkingStateChangedHandle = VoiceInterface->AddOnPlayerTalkingStateChangedDelegate_Handle(
		FOnPlayerTalkingStateChangedDelegate::CreateUObject(this, &ThisClass::HandleVoiceTalkingStateChanged));
}

void AHeistPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (VoiceTalkingStateChangedHandle.IsValid())
	{
		IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
		if (OSS != nullptr)
		{
			IOnlineVoicePtr VoiceInterface = OSS->GetVoiceInterface();
			if (VoiceInterface.IsValid())
			{
				VoiceInterface->ClearOnPlayerTalkingStateChangedDelegate_Handle(VoiceTalkingStateChangedHandle);
			}
		}
		VoiceTalkingStateChangedHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void AHeistPlayerController::HandleVoiceTalkingStateChanged(FUniqueNetIdRef PlayerId, bool bIsTalking)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (OSS == nullptr) return;

	IOnlineIdentityPtr IdentityInterface = OSS->GetIdentityInterface();
	if (!IdentityInterface.IsValid()) return;

	TSharedPtr<const FUniqueNetId> LocalPlayerId = IdentityInterface->GetUniquePlayerId(0);
	if (!LocalPlayerId.IsValid() || *LocalPlayerId != *PlayerId) return;

	FHeistVoiceTalkingStateMessage Message;
	Message.PlayerState = GetPlayerState<APlayerState>();
	Message.bIsTalking = bIsTalking;

	UHeistMessageSubsystem::Get(this).BroadcastMessage(
		HeistMessageTags::Message_Voice_TalkingStateChanged, Message);
}

void AHeistPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	AHeistPlayerState* HeistPS = GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS)) return;

	UHeistAbilitySystemComponent* ASC = HeistPS->GetHeistAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	if (!ASC->HasMatchingGameplayTag(HeistStateTags::State_RotationDisabled))
	{
		UpdateCursorRotation();
	}

	ASC->ProcessAbilityInput(DeltaTime, false);
}

void AHeistPlayerController::UpdateCursorRotation()
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn)) return;

	FVector RayOrigin;
	FVector RayDirection;
	if (!DeprojectMousePositionToWorld(RayOrigin, RayDirection)) return;

	if (FMath::IsNearlyZero(RayDirection.Z)) return;

	float GroundZ = ControlledPawn->GetActorLocation().Z;
	float T = (GroundZ - RayOrigin.Z) / RayDirection.Z;
	if (T < 0.0f) return;

	FVector CursorWorldPosition = RayOrigin + RayDirection * T;

	FVector LookDirection = CursorWorldPosition - ControlledPawn->GetActorLocation();
	LookDirection.Z = 0.0f;

	if (LookDirection.IsNearlyZero()) return;

	SetControlRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
}
