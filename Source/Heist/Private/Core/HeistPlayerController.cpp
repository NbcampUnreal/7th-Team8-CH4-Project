#include "Core/HeistPlayerController.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Core/HeistLobbyGameMode.h"
#include "Character/HeistTags_State.h"
#include "Components/HeistSpectatorControllerComponent.h"
#include "Core/HeistMatchGameMode.h"
#include "Core/HeistPlayerState.h"
#include "Voice/HeistVoipTalker.h"
#include "Voice/HeistVoiceSubsystem.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "GameFramework/Pawn.h"
#include "Components/AudioComponent.h"
#include "EnhancedInputComponent.h"
#include "OnlineSubsystem.h"
#include "Interfaces/VoiceInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "DrawDebugHelpers.h"
#include "Core/HeistMatchGameState.h"
#include "GameFramework/GameStateBase.h"
#include "Sound/SoundAttenuation.h"


AHeistPlayerController::AHeistPlayerController()
{
	bShowMouseCursor = true;
	SpectatorControllerComponent = CreateDefaultSubobject<UHeistSpectatorControllerComponent>(TEXT("SpectatorControllerComponent"));
}

void AHeistPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bSentReadyForBriefingStart = false;
	bSentReadyForMatchTravel = false;

	// 클라에서는 브리핑 컨텍스트 복제 순서를 신뢰할 수 없으므로, PlayerController 훅마다 readiness를 다시 두드린다.
	UE_LOG(LogTemp, Log, TEXT("[BriefingRetry] BeginPlay: PC=%s Local=%d"), *GetNameSafe(this), IsLocalController() ? 1 : 0);
	TryReportReadyForBriefingStart();
	TryNotifyBriefingContextReady();

	if (IsLocalController())
	{
		// 정상 경로에서는 이미 보이스가 켜져 있으므로 no-op이다.
		// 이 리스너는 브리핑 진입 레이스 등으로 시작을 놓친 경우를 복구하기 위한 안전장치다.
		PhaseChangedListenerHandle = UHeistMessageSubsystem::Get(this).RegisterListener<FHeistPhaseChangedMessage>(
			HeistMessageTags::Message_Phase_Changed,
			[this](FGameplayTag, const FHeistPhaseChangedMessage& Msg)
			{
				if (Msg.CurrentPhase == EHeistMatchPhase::Briefing ||
					Msg.CurrentPhase == EHeistMatchPhase::Execution)
				{
					if (UHeistVoiceSubsystem* VS = GetGameInstance()->GetSubsystem<UHeistVoiceSubsystem>())
					{
						VS->RequestVoiceRefresh(this);
					}
				}
			});
	}
}

void AHeistPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsValid(SpectatorControllerComponent))
	{
		SpectatorControllerComponent->BindInput(InputComponent);
	}

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

	UE_LOG(LogTemp, Log, TEXT("[BriefingRetry] AcknowledgePossession: PC=%s Pawn=%s"),
		*GetNameSafe(this),
		*GetNameSafe(NewPawn));
	SetAudioListenerOverride(NewPawn->GetRootComponent(), FVector::ZeroVector, FRotator::ZeroRotator);

	TryNotifyBriefingContextReady();

	if (UHeistVoiceSubsystem* VS = GetGameInstance()->GetSubsystem<UHeistVoiceSubsystem>())
	{
		VS->OnPawnPossessed(this, NewPawn);
	}

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

void AHeistPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	APlayerState* CurrentPlayerState = GetPlayerState<APlayerState>();
	UE_LOG(LogTemp, Log, TEXT("[BriefingRetry] OnRep_PlayerState: PC=%s PS=%s"),
		*GetNameSafe(this),
		*GetNameSafe(CurrentPlayerState));
	TryReportReadyForBriefingStart();
	TryNotifyBriefingContextReady();

	if (IsLocalController())
	{
		if (UHeistVoiceSubsystem* VS = GetGameInstance()->GetSubsystem<UHeistVoiceSubsystem>())
		{
			VS->OnPlayerStateReady(this);
		}
	}
}

void AHeistPlayerController::SeamlessTravelTo(APlayerController* NewPC)
{
	PrepareForMatchTravelAudioShutdown();

	if (UHeistVoiceSubsystem* VS = GetGameInstance()->GetSubsystem<UHeistVoiceSubsystem>())
	{
		VS->BeginTravelShutdown(GetWorld());
	}

	Super::SeamlessTravelTo(NewPC);
}

void AHeistPlayerController::NotifyLoadedWorld(FName WorldPackageName, bool bFinalDest)
{
	PrepareForMatchTravelAudioShutdown();
	Super::NotifyLoadedWorld(WorldPackageName, bFinalDest);

	if (bFinalDest)
	{
		bSentReadyForBriefingStart = false;
		bSentReadyForMatchTravel = false;
		TryReportReadyForBriefingStart();

		if (IsLocalController())
		{
			UHeistMessageSubsystem::Get(this).BroadcastMessage(
				HeistMessageTags::Message_Travel_SeamlessEnd, FHeistTravelSeamlessEndMessage{});
		}

		if (UHeistVoiceSubsystem* VS = GetGameInstance()->GetSubsystem<UHeistVoiceSubsystem>())
		{
			VS->TryRecoverVoiceForPlayer(this);
		}
	}
}

void AHeistPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PrepareForMatchTravelAudioShutdown();

	if (PhaseChangedListenerHandle.IsValid())
	{
		PhaseChangedListenerHandle.Unregister();
	}

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

void AHeistPlayerController::StartVoiceCapture()
{
	if (bVoiceCaptureActive)
	{
		return;
	}

	StartTalking();
	bVoiceCaptureActive = true;
}

void AHeistPlayerController::StopVoiceCapture()
{
	if (!bVoiceCaptureActive)
	{
		return;
	}

	StopTalking();
	bVoiceCaptureActive = false;
}

void AHeistPlayerController::TryStartVoiceCapture()
{
	if (UHeistVoiceSubsystem* VS = GetGameInstance()->GetSubsystem<UHeistVoiceSubsystem>())
	{
		VS->RequestVoiceRefresh(this);
	}
}

void AHeistPlayerController::PrepareForMatchTravelAudioShutdown()
{
	StopVoiceCapture();

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

	ClearAudioListenerOverride();
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

void AHeistPlayerController::DrawVoiceRangeDebug()
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn)) return;

	AHeistPlayerState* HeistPS = GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS)) return;

	UHeistVoipTalker* VoipTalker = HeistPS->GetVoipTalker();
	if (!IsValid(VoipTalker) || !IsValid(VoipTalker->Settings.AttenuationSettings)) return;

	const FSoundAttenuationSettings& Attenuation = VoipTalker->Settings.AttenuationSettings->Attenuation;
	const FVector Center = ControlledPawn->GetActorLocation();
	const float InnerRadius = Attenuation.AttenuationShapeExtents.X;
	const float OuterRadius = InnerRadius + Attenuation.FalloffDistance;

	DrawDebugCircle(GetWorld(), Center, InnerRadius, 64, FColor::Green, false, -1.f, 0, 3.f, FVector::ForwardVector, FVector::RightVector);
	DrawDebugCircle(GetWorld(), Center, OuterRadius, 64, FColor::Red,   false, -1.f, 0, 3.f, FVector::ForwardVector, FVector::RightVector);
}

bool AHeistPlayerController::IsInMatchBriefingPhase() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const AHeistMatchGameState* MatchGameState = World->GetGameState<AHeistMatchGameState>();
	if (!IsValid(MatchGameState))
	{
		return false;
	}

	return MatchGameState->IsBriefingPhase();
}

bool AHeistPlayerController::CanReportReadyForBriefingStart() const
{
	if (!IsLocalController())
	{
		return false;
	}

	if (bSentReadyForBriefingStart)
	{
		return false;
	}

	if (!IsValid(GetPlayerState<APlayerState>()))
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const FString MapName = World->GetMapName();
	if (MapName.Contains(TEXT("Transition"), ESearchCase::IgnoreCase))
	{
		return false;
	}

	const AHeistMatchGameState* MatchGameState = World->GetGameState<AHeistMatchGameState>();
	if (!IsValid(MatchGameState))
	{
		return false;
	}

	if (MatchGameState->IsExecutionPhase() || MatchGameState->IsBriefingPhase())
	{
		return false;
	}

	return true;
}

void AHeistPlayerController::TryReportReadyForBriefingStart()
{
	if (!CanReportReadyForBriefingStart())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[BriefingReady] ReportReadyForBriefingStart: PC=%s PS=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetPlayerState<APlayerState>()));
	ServerNotifyReadyForBriefingStart();
	bSentReadyForBriefingStart = true;
}

void AHeistPlayerController::TryNotifyBriefingContextReady()
{
	if (!IsLocalController())
	{
		return;
	}
	if (!IsInMatchBriefingPhase())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[BriefingRetry] Skip: not in briefing phase PC=%s"), *GetNameSafe(this));
		return;
	}

	AHeistPlayerState* HeistPS = GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[BriefingRetry] Skip: PlayerState invalid PC=%s"), *GetNameSafe(this));
		return;
	}

	UHeistBriefingPlayerComponent* BriefingComp = HeistPS->GetBriefingPlayerComponent();
	if (!IsValid(BriefingComp))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[BriefingRetry] Skip: BriefingComponent invalid PS=%s"), *HeistPS->GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[BriefingRetry] TryNotify: PC=%s PS=%s Team=%d Pawn=%s"),
		*GetNameSafe(this),
		*HeistPS->GetName(),
		static_cast<int32>(HeistPS->GetAssignedTeam()),
		*GetNameSafe(GetPawn()));
	BriefingComp->BroadcastContextReady();
}

void AHeistPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bShowVoiceRange)
	{
		DrawVoiceRangeDebug();
	}

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

void AHeistPlayerController::ServerRequestSetReady_Implementation(bool bReady)
{
	AHeistPlayerState* HeistPS = GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS)) return;

	HeistPS->SetIsReady(bReady);
}

void AHeistPlayerController::ServerNotifyReadyForBriefingStart_Implementation()
{
	AHeistMatchGameMode* HeistGM = GetWorld() ? GetWorld()->GetAuthGameMode<AHeistMatchGameMode>() : nullptr;
	if (!IsValid(HeistGM))
	{
		return;
	}

	HeistGM->NotifyPlayerReadyForBriefingStart(this);
}

void AHeistPlayerController::ServerNotifyReadyForMatchTravel_Implementation()
{
	if (AHeistLobbyGameMode* LobbyGM = GetWorld() ? GetWorld()->GetAuthGameMode<AHeistLobbyGameMode>() : nullptr; IsValid(LobbyGM))
	{
		LobbyGM->NotifyPlayerReadyForMatchTravel(this);
		return;
	}

	if (AHeistMatchGameMode* MatchGM = GetWorld() ? GetWorld()->GetAuthGameMode<AHeistMatchGameMode>() : nullptr; IsValid(MatchGM))
	{
		MatchGM->NotifyPlayerReadyForMatchTravel(this);
		return;
	}
}

void AHeistPlayerController::ClientPrepareForMatchTravel_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("[VoiceTravel] PrepareForMatchTravel: PC=%s Local=%d"),
		*GetNameSafe(this),
		IsLocalController() ? 1 : 0);

	PrepareForMatchTravelAudioShutdown();

	if (UHeistVoiceSubsystem* VS = GetGameInstance()->GetSubsystem<UHeistVoiceSubsystem>())
	{
		VS->BeginTravelShutdown(GetWorld());
	}

	if (IsLocalController())
	{	if (UHeistMessageSubsystem* MS = UHeistMessageSubsystem::TryGet(this))
		{
			MS->BroadcastMessage(
						  HeistMessageTags::Message_Travel_SeamlessStart, FHeistTravelSeamlessStartMessage{});
		}

		if (!bSentReadyForMatchTravel)
		{
			ServerNotifyReadyForMatchTravel();
			bSentReadyForMatchTravel = true;
		}
	}
}

void AHeistPlayerController::ClientEndBriefingPresentation_Implementation()
{
	UHeistMessageSubsystem::Get(this).BroadcastMessage(
		HeistMessageTags::Message_Briefing_End, FHeistBriefingEndMessage{});
}

void AHeistPlayerController::ClientNotifyArrested_Implementation()
{
	if (!IsValid(SpectatorControllerComponent)) return;

	SpectatorControllerComponent->EnterArrestSpectating();
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
