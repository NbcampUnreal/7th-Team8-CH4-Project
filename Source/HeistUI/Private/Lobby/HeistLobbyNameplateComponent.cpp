#include "Lobby/HeistLobbyNameplateComponent.h"

#include "Lobby/HeistLobbyNameplateWidget.h"
#include "Character/HeistCharacter.h"
#include "Core/HeistPlayerState.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "GameFramework/Pawn.h"

UHeistLobbyNameplateComponent::UHeistLobbyNameplateComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRelativeLocation(FVector(0.f, 0.f, 200.f));
}

void UHeistLobbyNameplateComponent::BeginPlay()
{
	Super::BeginPlay();

	AHeistCharacter* OwnerCharacter = Cast<AHeistCharacter>(GetOwner());
	if (IsValid(OwnerCharacter))
	{
		PlayerStateInitializedHandle = OwnerCharacter->OnPlayerStateInitialized.AddUObject(
			this, &UHeistLobbyNameplateComponent::InitializeWithPlayerState
		);
	}

	// 서버(PossessedBy)에서는 BeginPlay 시점에 이미 delegate가 broadcast된 이후일 수 있어
	// 직접 확인한다. InitializeWithPlayerState 내부에서 중복 처리를 막는다.
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		InitializeWithPlayerState(OwnerPawn->GetPlayerState<AHeistPlayerState>());
	}
}

void UHeistLobbyNameplateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AHeistCharacter* OwnerCharacter = Cast<AHeistCharacter>(GetOwner()))
	{
		OwnerCharacter->OnPlayerStateInitialized.Remove(PlayerStateInitializedHandle);
	}

	ReadyStateHandle.Unregister();
	PlayersChangedHandle.Unregister();

	Super::EndPlay(EndPlayReason);
}

void UHeistLobbyNameplateComponent::InitializeWithPlayerState(AHeistPlayerState* PlayerState)
{
	if (!IsValid(PlayerState)) return;
	if (BoundPlayerState.IsValid()) return;

	BoundPlayerState = PlayerState;

	// 초기화 완료 — 델리게이트 구독 해제
	if (AHeistCharacter* OwnerCharacter = Cast<AHeistCharacter>(GetOwner()))
	{
		OwnerCharacter->OnPlayerStateInitialized.Remove(PlayerStateInitializedHandle);
	}

	if (UHeistMessageSubsystem* MessageSubsystem = UHeistMessageSubsystem::TryGet(this))
	{
		ReadyStateHandle = MessageSubsystem->RegisterListener<FHeistLobbyReadyStateChangedMessage>(
			HeistMessageTags::Message_Lobby_ReadyStateChanged,
			[this](FGameplayTag Channel, const FHeistLobbyReadyStateChangedMessage& Message)
			{
				RefreshNameplate();
			});

		PlayersChangedHandle = MessageSubsystem->RegisterListener<FHeistLobbyPlayersChangedMessage>(
			HeistMessageTags::Message_Lobby_PlayersChanged,
			[this](FGameplayTag Channel, const FHeistLobbyPlayersChangedMessage& Message)
			{
				RefreshNameplate();
			});
	}

	RefreshNameplate();
}

void UHeistLobbyNameplateComponent::RefreshNameplate()
{
	AHeistPlayerState* PlayerState = BoundPlayerState.Get();
	if (!IsValid(PlayerState)) return;

	UHeistLobbyNameplateWidget* NameplateWidget = Cast<UHeistLobbyNameplateWidget>(GetWidget());
	if (!IsValid(NameplateWidget)) return;

	NameplateWidget->UpdateNameplate(
		PlayerState->GetPlayerName(),
		PlayerState->GetIsReady(),
		PlayerState->GetIsHost()
	);
}
