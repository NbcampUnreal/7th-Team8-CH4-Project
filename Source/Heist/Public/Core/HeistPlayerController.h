#pragma once

#include "CoreMinimal.h"
#include "Core/HeistMatchTypes.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistPlayerController.generated.h"

class UHeistBriefingPlayerComponent;
class UHeistSpectatorControllerComponent;
class AHeistBriefingDrawingBoard;
class UHeistWallAvoidanceCameraModifier;

UCLASS()
class HEIST_API AHeistPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHeistPlayerController();

	UFUNCTION(Server, Reliable)
	void ServerRequestSetReady(bool bReady);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerRequestTogglePreviewCharacter();

	UFUNCTION(Server, Reliable)
	void ServerNotifyReadyForBriefingStart();

	UFUNCTION(Server, Reliable)
	void ServerNotifyReadyForMatchTravel();

	UFUNCTION(Client, Reliable)
	void ClientPrepareForMatchTravel();

	UFUNCTION(Client, Reliable)
	void ClientEndBriefingPresentation();

	UFUNCTION(Client, Reliable)
	void ClientNotifyArrested();

	UFUNCTION(Client, Reliable)
	void ClientBeginVehicleEscapeCinematic(int32 GroupIndex);

	UFUNCTION(Client, Reliable)
	void ClientNotifyMatchResult(EHeistTeam WinnerTeam, EHeistVictoryReason Reason);

	void TryNotifyBriefingContextReady();

	// 서브시스템이 최종 판단 후 실행을 위임 — StartTalking()/StopTalking()은 PC만 호출 가능
	void StartVoiceCapture();
	void StopVoiceCapture();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void AcknowledgePossession(APawn* NewPawn) override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SeamlessTravelTo(APlayerController* NewPC) override;
	// 이것은 클라이언트 측 Transient 컴포넌트를 정리합니다, HeistPlayerState::SeamlessTravelTo()는 서버만 정리됩니다.
	virtual void NotifyLoadedWorld(FName WorldPackageName, bool bFinalDest) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Heist|GameEnd")
	void BP_OnVehicleEscapeCinematicStarted(int32 GroupIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Heist|GameEnd")
	void BP_OnMatchResultReceived(EHeistTeam WinnerTeam, EHeistVictoryReason Reason);

private:
	void TryStartVoiceCapture();
	void PrepareForMatchTravelAudioShutdown();
	void TryReportReadyForBriefingStart();
	bool CanReportReadyForBriefingStart() const;

	void UpdateCursorRotation();
	void HandleVoiceTalkingStateChanged(FUniqueNetIdRef PlayerId, bool bIsTalking);

	UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (MetaClass = "/Script/UMG.UserWidget"))
	TObjectPtr<UClass> PlayHUDClass;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SystemMenuInputAction;

	UPROPERTY(VisibleAnywhere, Category = "Heist|Components")
	TObjectPtr<UHeistSpectatorControllerComponent> SpectatorControllerComponent;

	void Input_SystemMenu();

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UHeistWallAvoidanceCameraModifier> WallAvoidanceCameraModifierClass;

	UPROPERTY(EditDefaultsOnly, Category = "Voice|Debug")
	bool bShowVoiceRange = false;

	void DrawVoiceRangeDebug();

	FDelegateHandle VoiceTalkingStateChangedHandle;
	FHeistMessageListenerHandle PhaseChangedListenerHandle;

	bool IsInMatchBriefingPhase() const;
	bool bSentReadyForBriefingStart = false;
	bool bSentReadyForMatchTravel = false;
	bool bVoiceCaptureActive = false;
};
