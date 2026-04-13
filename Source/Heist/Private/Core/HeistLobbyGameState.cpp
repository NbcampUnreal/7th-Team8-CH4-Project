#include "Core/HeistLobbyGameState.h"
#include "Core/HeistPlayerState.h"

#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"

void AHeistLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	BroadcastPlayersChanged();
}

void AHeistLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	BroadcastPlayersChanged();
}

void AHeistLobbyGameState::BroadcastPlayersChanged()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!IsValid(GameInstance)) return;

	UHeistMessageSubsystem* Subsystem = GameInstance->GetSubsystem<UHeistMessageSubsystem>();
	if (!IsValid(Subsystem)) return;

	Subsystem->BroadcastMessage(HeistMessageTags::Message_Lobby_PlayersChanged, FHeistLobbyPlayersChangedMessage{});
}

void AHeistLobbyGameState::SetInviteCode(const FString& NewInviteCode)
{
	InviteCode = NewInviteCode;
	BroadcastInviteCodeChanged();
}

void AHeistLobbyGameState::OnRep_InviteCode()
{
	BroadcastInviteCodeChanged();
}

void AHeistLobbyGameState::BroadcastInviteCodeChanged()
{
	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);

	FHeistLobbyInviteCodeChangedMessage Message;
	Message.InviteCode = InviteCode;

	MessageSubsystem.BroadcastMessage(HeistMessageTags::Message_Lobby_InviteCodeChanged, Message);
}

bool AHeistLobbyGameState::AreAllPlayersReady() const
{
	if (PlayerArray.IsEmpty()) return false;

	for (APlayerState* PlayerState : PlayerArray)
	{
		const AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPS) || !HeistPS->GetIsReady()) return false;
	}

	return true;
}

void AHeistLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeistLobbyGameState, InviteCode);
}
