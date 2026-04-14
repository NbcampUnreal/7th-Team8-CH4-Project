#pragma once

#include "GameFramework/PlayerState.h"
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
