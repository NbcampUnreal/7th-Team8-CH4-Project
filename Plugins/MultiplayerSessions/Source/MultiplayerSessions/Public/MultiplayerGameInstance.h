#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionTypes.h"
#include "MultiplayerGameInstance.generated.h"

/**
 * Optional base GameInstance that handles session travel.
 * Supports both listen-server and dedicated-server lobby modes, controlled by bUseDedicatedServer.
 *
 * Listen-server mode (bUseDedicatedServer = false):
 *   Session Created → ServerTravel(LobbyPath?listen)        host becomes the listen server
 *   Session Joined  → ClientTravel(resolved Steam address)  client connects to the host
 *   Session Started → RequestDedicatedServerAddress(Game)
 *                   → OnDedicatedServerAddressReceived      ServerTravel to dedicated game server
 *
 * Dedicated-server mode (bUseDedicatedServer = true):
 *   Session Created → RequestDedicatedServerAddress(Lobby)
 *                   → OnDedicatedServerAddressReceived      ServerTravel to dedicated lobby
 *   Session Joined  → ClientTravel(resolved Steam address)  client connects to the dedicated lobby
 *   Session Started → RequestDedicatedServerAddress(Game)
 *                   → OnDedicatedServerAddressReceived      ServerTravel to dedicated game server
 *
 * Usage:
 *   1. Set your project's GameInstance class to this (or a Blueprint subclass).
 *   2. Set LobbyPath in Blueprint Class Defaults.
 *   3. Override RequestDedicatedServerAddress() to query your matchmaking service.
 *      For testing, set DedicatedServerAddress directly in Class Defaults.
 *
 * Override HandleSessionCreated / HandleSessionJoined / HandleSessionStarted
 * for fully custom travel behavior.
 *
 * NOTE — Switching between modes:
 *   - Switching to dedicated mode requires DedicatedServerAddress (testing) or a
 *     RequestDedicatedServerAddress() override. Both Lobby and Game requests share the
 *     same DedicatedServerAddress field; if different addresses are needed per purpose,
 *     override RequestDedicatedServerAddress() and branch on the Purpose argument.
 *   - HandleSessionJoined is identical in both modes: it always ClientTravels to the
 *     Steam-resolved address, which automatically points to the right server.
 *   - ArenaGameMode behaves identically in both modes; no changes needed there.
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// Map path for the lobby (used as listen-server map or as the dedicated lobby map path).
	// In listen-server mode: the host ServerTravels here with ?listen appended.
	// In dedicated-server mode: passed to RequestDedicatedServerAddress(Lobby) for routing.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Multiplayer|Lobby")
	FString LobbyPath = TEXT("/Game/Maps/Lobby");

	// When true, the lobby runs on a dedicated server and game server travel also uses
	// RequestDedicatedServerAddress(). When false, the lobby is a listen server.
	// Default: true (dedicated server).
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer|Lobby")
	bool bUseDedicatedServer = true;

	// If true, destroys the lobby session when the game starts.
	// Use for QuickMatch. For custom lobbies, set false to allow returning.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Multiplayer|Game")
	bool bDestroySessionOnStart = false;

	// Dedicated game server address for testing only (e.g. 127.0.0.1:7778).
	// Used by the default RequestDedicatedServerAddress() implementation for both Lobby and Game.
	// In production, override RequestDedicatedServerAddress() instead.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Multiplayer|Game|Development")
	FString DedicatedServerAddress;

protected:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION()
	virtual void HandleSessionCreated(bool bWasSuccessful);

	virtual void HandleSessionJoined(EOnJoinSessionCompleteResult::Type Result);

	UFUNCTION()
	virtual void HandleSessionStarted(bool bWasSuccessful);

	// Override this to query your matchmaking service for a dedicated server address.
	// Purpose indicates whether the address is needed for the lobby or the game server.
	// Call OnDedicatedServerAddressReceived() when the address is ready.
	// Default implementation uses DedicatedServerAddress (for development/testing).
	virtual void RequestDedicatedServerAddress(EMultiplayerTravelPurpose Purpose);

	// Call this when the dedicated server address is ready.
	// Applies session cleanup and ServerTravel based on Purpose.
	void OnDedicatedServerAddressReceived(const FString& Address, EMultiplayerTravelPurpose Purpose);

private:
	void UnbindDelegates();
};
