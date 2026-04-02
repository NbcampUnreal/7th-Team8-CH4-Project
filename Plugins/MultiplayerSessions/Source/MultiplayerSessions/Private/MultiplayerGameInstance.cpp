#include "MultiplayerGameInstance.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/World.h"

void UMultiplayerGameInstance::Init()
{
	Super::Init();

	if (UMultiplayerSessionsSubsystem* Subsystem = GetSubsystem<UMultiplayerSessionsSubsystem>())
	{
		Subsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::HandleSessionCreated);
		Subsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::HandleSessionJoined);
		Subsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::HandleSessionStarted);
	}
}

void UMultiplayerGameInstance::Shutdown()
{
	UnbindDelegates();
	Super::Shutdown();
}

void UMultiplayerGameInstance::UnbindDelegates()
{
	if (UMultiplayerSessionsSubsystem* Subsystem = GetSubsystem<UMultiplayerSessionsSubsystem>())
	{
		Subsystem->MultiplayerOnCreateSessionComplete.RemoveDynamic(this, &ThisClass::HandleSessionCreated);
		Subsystem->MultiplayerOnJoinSessionComplete.RemoveAll(this);
		Subsystem->MultiplayerOnStartSessionComplete.RemoveDynamic(this, &ThisClass::HandleSessionStarted);
	}
}

void UMultiplayerGameInstance::HandleSessionCreated(bool bWasSuccessful)
{
	if (!bWasSuccessful) return;

	if (bUseDedicatedServer)
	{
		// Dedicated-server mode: request the lobby server address from the matchmaking service.
		// OnDedicatedServerAddressReceived will ServerTravel to that address.
		RequestDedicatedServerAddress(EMultiplayerTravelPurpose::Lobby);
	}
	else
	{
		// Listen-server mode: this machine becomes the lobby server.
		UWorld* World = GetWorld();
		if (!World) return;

		ensureAlwaysMsgf(!LobbyPath.IsEmpty(), TEXT("UMultiplayerGameInstance: LobbyPath is not set."));
		World->ServerTravel(LobbyPath + TEXT("?listen"));
	}
}

void UMultiplayerGameInstance::HandleSessionJoined(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success) return;

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem) return;

	IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	FString Address;
	if (SessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
	{
		if (APlayerController* PC = GetFirstLocalPlayerController())
		{
			PC->ClientTravel(Address, TRAVEL_Absolute);
		}
	}
}

void UMultiplayerGameInstance::HandleSessionStarted(bool bWasSuccessful)
{
	if (!bWasSuccessful) return;

	// Both listen-server and dedicated-server modes travel to a dedicated game server on start.
	RequestDedicatedServerAddress(EMultiplayerTravelPurpose::Game);
}

void UMultiplayerGameInstance::RequestDedicatedServerAddress(EMultiplayerTravelPurpose Purpose)
{
	// Default implementation for development/testing.
	// Override this in your subclass to query a matchmaking service.
	// If lobby and game server addresses differ, branch on Purpose here.
	ensureAlwaysMsgf(!DedicatedServerAddress.IsEmpty(),
		TEXT("UMultiplayerGameInstance: Override RequestDedicatedServerAddress() or set DedicatedServerAddress for testing."));

	OnDedicatedServerAddressReceived(DedicatedServerAddress, Purpose);
}

void UMultiplayerGameInstance::OnDedicatedServerAddressReceived(const FString& Address, EMultiplayerTravelPurpose Purpose)
{
	if (Address.IsEmpty()) return;

	// Only destroy the session when traveling to the game server, not the lobby.
	// Destroying the lobby session would make the session unavailable for joining clients.
	if (Purpose == EMultiplayerTravelPurpose::Game && bDestroySessionOnStart)
	{
		if (UMultiplayerSessionsSubsystem* Subsystem = GetSubsystem<UMultiplayerSessionsSubsystem>())
		{
			Subsystem->DestroySession();
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(Address);
	}
}
