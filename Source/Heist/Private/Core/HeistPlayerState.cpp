#include "Core/HeistPlayerState.h"

#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "AbilitySystem/HeistAttributeSet.h"
#include "Components/HeistBriefingPlayerComponent.h"

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

	BriefingPlayerComponent = CreateDefaultSubobject<UHeistBriefingPlayerComponent>(TEXT("BriefingPlayerComponent"));

	// PlayerState는 NetUpdateFrequency 기본값이 낮으므로 GAS 반응성을 위해 높인다.
	SetNetUpdateFrequency(100.0f);
}

void AHeistPlayerState::BeginPlay()
{
	Super::BeginPlay();
	BindVoipTalker();
}

void AHeistPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindVoipTalker();
	Super::EndPlay(EndPlayReason);
}

void AHeistPlayerState::SeamlessTravelTo(APlayerState* NewPlayerState)
{
	// carry-over 직전에 VoipListenerSynthComponent를 해제한다.
	// EndPlay()는 SeamlessTravel에서 구 World 클린업 후에 호출되므로 타이밍이 늦다.
	UnbindVoipTalker();
	Super::SeamlessTravelTo(NewPlayerState);
}

void AHeistPlayerState::OnSetUniqueId()
{
	Super::OnSetUniqueId();
	BindVoipTalker();
}

void AHeistPlayerState::BindVoipTalker()
{
	if (!IsValid(VoipTalker)) return;
	VoipTalker->RegisterWithPlayerState(this);
}

void AHeistPlayerState::UnbindVoipTalker()
{
	UVOIPStatics::ResetPlayerVoiceTalker(this);
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
	DOREPLIFETIME(AHeistPlayerState, AssignedTeam);
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

void AHeistPlayerState::SetAssignedTeam(EHeistTeam InTeam)
{
	AssignedTeam = InTeam;
}
