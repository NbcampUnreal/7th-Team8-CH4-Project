#include "Core/HeistPlayerState.h"

#include "VoipListenerSynthComponent.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "AbilitySystem/HeistAttributeSet.h"
#include "Components/HeistBriefingPlayerComponent.h"
#include "Core/HeistPlayerController.h"

#include "Voice/HeistVoipTalker.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "Net/UnrealNetwork.h"

// 액터 소유 컴포넌트 정리 (일반 케이스)
static void SafeUnregisterVoipListenerComp(AActor* Owner)
{
	if (!Owner) return;

	TArray<UVoipListenerSynthComponent*> Comps;
	Owner->GetComponents<UVoipListenerSynthComponent>(Comps);

	for (UVoipListenerSynthComponent* Comp : Comps)
	{
		if (Comp && Comp->IsRegistered())
		{
			Comp->UnregisterComponent();
		}
	}
}

// /Engine/Transient에 떠있는 미소속 컴포넌트 정리
static void SafeUnregisterTransientVoipComps()
{
	for (TObjectIterator<UVoipListenerSynthComponent> It; It; ++It)
	{
		UVoipListenerSynthComponent* Comp = *It;
		if (Comp && Comp->IsRegistered() && Comp->GetWorld() == nullptr)
		{
			Comp->UnregisterComponent();
		}
	}
}

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
	SafeUnregisterVoipListenerComp(this);

	UnbindVoipTalker();
	Super::EndPlay(EndPlayReason);
}

void AHeistPlayerState::SeamlessTravelTo(APlayerState* NewPlayerState)
{
	SafeUnregisterVoipListenerComp(this);
	SafeUnregisterTransientVoipComps();

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

void AHeistPlayerState::OnRep_AssignedTeam()
{
	UE_LOG(LogTemp, Log, TEXT("[AssignedTeam] OnRep: PS=%s Team=%d PC=%s"),
		*GetNameSafe(this),
		static_cast<int32>(AssignedTeam),
		*GetNameSafe(GetPlayerController()));

	APlayerController* PC = GetPlayerController();
	if (!IsValid(PC) || !PC->IsLocalController())
	{
		UE_LOG(LogTemp, Log, TEXT("[AssignedTeam] Defer: local PC not ready for PS=%s"), *GetNameSafe(this));
		return;
	}

	AHeistPlayerController* HeistPC = Cast<AHeistPlayerController>(PC);
	if (!IsValid(HeistPC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AssignedTeam] Defer: controller is not AHeistPlayerController PS=%s PC=%s"),
			*GetNameSafe(this),
			*PC->GetName());
		return;
	}

	// 팀 복제는 브리핑 컨텍스트 준비의 첫 퍼즐 조각이다.
	// 클라에서는 폰/컨트롤러/보드가 아직 없을 수 있으므로, 여기서는 최종 처리 대신 재시도만 건다.
	HeistPC->TryNotifyBriefingContextReady();
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
	if (AssignedTeam == InTeam)
	{
		return;
	}

	AssignedTeam = InTeam;
	OnRep_AssignedTeam();
}
