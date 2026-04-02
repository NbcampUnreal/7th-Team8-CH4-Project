#pragma once

#include "CoreMinimal.h"
#include "MultiplayerSessionTypes.generated.h"

namespace MultiplayerSessionConstants
{
	// Session settings key for match type filtering
	static const FName MatchTypeKey(TEXT("MatchType"));

	// Session settings key for invite code
	static const FName InviteCodeKey(TEXT("InviteCode"));
}

// Indicates the purpose of a dedicated server address request.
// Passed through RequestDedicatedServerAddress → OnDedicatedServerAddressReceived
// so each call site can apply the correct travel behavior.
UENUM(BlueprintType)
enum class EMultiplayerTravelPurpose : uint8
{
	Lobby  UMETA(DisplayName = "Lobby"),
	Game   UMETA(DisplayName = "Game")
};

USTRUCT(BlueprintType)
struct MULTIPLAYERSESSIONS_API FMultiplayerSessionConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumPublicConnections = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MatchType = TEXT("FREE");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxSearchResults = 10000;
};
