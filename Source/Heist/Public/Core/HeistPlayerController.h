#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "HeistPlayerController.generated.h"

class UHeistBriefingPlayerComponent;

UCLASS()
class HEIST_API AHeistPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHeistPlayerController();

	UFUNCTION(Server, Reliable)
	void ServerRequestSetReady(bool bReady);

	UFUNCTION(Client, Reliable)
	void ClientEndBriefingPresentation();

protected:
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void AcknowledgePossession(APawn* NewPawn) override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

private:
	void StartVoiceCapture();
	void StopVoiceCapture();

	void UpdateCursorRotation();
	void HandleVoiceTalkingStateChanged(FUniqueNetIdRef PlayerId, bool bIsTalking);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SystemMenuInputAction;

	void Input_SystemMenu();

	UPROPERTY(EditDefaultsOnly, Category = "Voice|Debug")
	bool bShowVoiceRange = false;

	void DrawVoiceRangeDebug();

	FDelegateHandle VoiceTalkingStateChangedHandle;

	/**
	 * BP_PlayerController에서 WBP_BriefingScreen 클래스를 할당한다.
	 * UUserWidget 참조 없이 UClass*로 보관해 UMG 종속을 제거한다.
	 * 실제 위젯 생성은 UHeistBriefingUISubsystem(HeistUI 모듈)이 담당한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Briefing", meta = (AllowedClasses = "UserWidget"))
	TObjectPtr<UClass> BriefingWidgetClass;

	void TryBindBriefingEventsFromPlayerState();
	void HandleBriefingContextReady();

	FDelegateHandle BriefingContextReadyHandle;
};
