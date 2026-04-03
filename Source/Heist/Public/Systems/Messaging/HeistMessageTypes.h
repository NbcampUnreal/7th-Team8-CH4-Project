#pragma once

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
