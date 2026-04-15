#pragma once

#include "Core/HeistMatchTypes.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "Components/HeistBriefingPlayerComponent.h"
#include "Core/HeistBriefingDrawingSyncComponent.h"
#include "HeistMessageTypes.generated.h"

USTRUCT()
struct FHeistSystemMenuToggleMessage
{
	GENERATED_BODY()
};

USTRUCT()
struct FHeistLobbyPlayersChangedMessage
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> PlayerNames;
};

USTRUCT()
struct FHeistLobbyInviteCodeChangedMessage
{
	GENERATED_BODY()

	UPROPERTY()
	FString InviteCode;
};

USTRUCT()
struct FHeistLobbyReadyStateChangedMessage
{
	GENERATED_BODY()
};

USTRUCT()
struct FHeistVoiceTalkingStateMessage
{
	GENERATED_BODY()

	// 발화 상태가 바뀐 플레이어
	UPROPERTY()
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY()
	bool bIsTalking = false;
};

USTRUCT()
struct FHeistPhaseChangedMessage
{
	GENERATED_BODY()

	UPROPERTY()
	EHeistMatchPhase CurrentPhase = EHeistMatchPhase::None;

	UPROPERTY()
	bool bBriefingSelectionLocked = false;
};

USTRUCT()
struct FHeistPhaseTimeUpdatedMessage
{
	GENERATED_BODY()

	UPROPERTY()
	float RemainingTime = 0.f;
};

USTRUCT(BlueprintType)
struct FHeistSoundDetectedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector OriginLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	float DetectionRadius = 0.0f;
};

USTRUCT(BlueprintType)
struct FHeistFlashlightAlertMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	bool bIsDetected = false;
};

USTRUCT()
struct FHeistBriefingContextReadyMessage
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UHeistBriefingPlayerComponent> BriefingPlayerComponent;

	UPROPERTY()
	TObjectPtr<UHeistBriefingDrawingSyncComponent> DrawingSyncComponent;

	EHeistBriefingViewMode ViewMode = EHeistBriefingViewMode::Thief;

	/** CreateWidget에 사용할 위젯 클래스. PC BP에서 할당한 값을 그대로 전달한다. */
	UPROPERTY()
	TObjectPtr<UClass> WidgetClass;
};

USTRUCT()
struct FHeistBriefingEndMessage
{
	GENERATED_BODY()
};
