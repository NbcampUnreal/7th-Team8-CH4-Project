#include "Core/HeistPlayerState.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "AbilitySystem/HeistAttributeSet.h"
#include "Voice/HeistVoipTalker.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "Net/UnrealNetwork.h"

AHeistPlayerState::AHeistPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UHeistAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UHeistAttributeSet>(TEXT("AttributeSet"));

	VoipTalker = CreateDefaultSubobject<UHeistVoipTalker>(TEXT("VoipTalker"));

	// PlayerState는 NetUpdateFrequency 기본값이 낮으므로 GAS 반응성을 위해 높인다.
	SetNetUpdateFrequency(100.0f);
}

void AHeistPlayerState::BeginPlay()
{
	Super::BeginPlay();

	VoipTalker->RegisterWithPlayerState(this);
}

void AHeistPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// World 정리 전에 VoipListenerSynthComponent가 Transient에 남지 않도록 해제한다.
	if (IsValid(VoipTalker))
	{
		VoipTalker->UnregisterFromPlayerState(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AHeistPlayerState::OnSetUniqueId()
{
	Super::OnSetUniqueId();

	if (IsValid(VoipTalker))
	{
		VoipTalker->RegisterWithPlayerState(this);
	}
}

void AHeistPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	BroadcastPlayersChanged();
}

void AHeistPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeistPlayerState, bIsReady);
	DOREPLIFETIME(AHeistPlayerState, bIsHost);
}

void AHeistPlayerState::SetIsReady(bool bNewIsReady)
{
	bIsReady = bNewIsReady;
	BroadcastReadyStateChanged();
}

void AHeistPlayerState::SetIsHost(bool bNewIsHost)
{
	bIsHost = bNewIsHost;
	BroadcastPlayersChanged();
}

void AHeistPlayerState::OnRep_bIsReady()
{
	BroadcastReadyStateChanged();
}

void AHeistPlayerState::OnRep_bIsHost()
{
	BroadcastPlayersChanged();
}

void AHeistPlayerState::BroadcastPlayersChanged() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!IsValid(GameInstance)) return;

	UHeistMessageSubsystem* Subsystem = GameInstance->GetSubsystem<UHeistMessageSubsystem>();
	if (!IsValid(Subsystem)) return;

	Subsystem->BroadcastMessage(HeistMessageTags::Message_Lobby_PlayersChanged, FHeistLobbyPlayersChangedMessage{});
}

void AHeistPlayerState::BroadcastReadyStateChanged() const
{
	UHeistMessageSubsystem::Get(this).BroadcastMessage(
		HeistMessageTags::Message_Lobby_ReadyStateChanged,
		FHeistLobbyReadyStateChangedMessage{});
}

UAbilitySystemComponent* AHeistPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UHeistAbilitySystemComponent* AHeistPlayerState::GetHeistAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UHeistVoipTalker* AHeistPlayerState::GetVoipTalker() const
{
	return VoipTalker;
}
