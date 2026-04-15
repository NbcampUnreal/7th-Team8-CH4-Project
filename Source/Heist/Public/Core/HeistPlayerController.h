#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "InputAction.h"
#include "Systems/HeistBriefingWidgetInterface.h"
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

private:
	void UpdateCursorRotation();
	void HandleVoiceTalkingStateChanged(FUniqueNetIdRef PlayerId, bool bIsTalking);
	void RemoveBriefingWidget();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SystemMenuInputAction;

	void Input_SystemMenu();

	UPROPERTY(EditDefaultsOnly, Category = "Voice|Debug")
	bool bShowVoiceRange = false;

	void DrawVoiceRangeDebug();

	FDelegateHandle VoiceTalkingStateChangedHandle;

	/**
	 * BP_PlayerController에서 WBP_BriefingMap 클래스를 할당한다.
	 * 할당할 위젯은 반드시 IHeistBriefingWidget을 구현해야 한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Briefing")
	TSubclassOf<UUserWidget> BriefingWidgetClass;

	/** 생성된 브리핑 위젯 인스턴스. 한 번만 생성되며 IHeistBriefingWidget으로 캐스트해 초기화한다. */
	UPROPERTY()
	TObjectPtr<UUserWidget> BriefingWidgetInstance;

	void TryBindBriefingEventsFromPlayerState();
	void HandleBriefingContextReady();

	FDelegateHandle BriefingContextReadyHandle;
};
