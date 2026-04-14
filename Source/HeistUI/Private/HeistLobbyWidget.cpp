#include "HeistLobbyWidget.h"

#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "Core/HeistLobbyGameMode.h"
#include "Core/HeistLobbyGameState.h"
#include "Core/HeistPlayerController.h"
#include "Core/HeistPlayerState.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformApplicationMisc.h"

bool UHeistLobbyWidget::Initialize()
{
	if (!Super::Initialize()) return false;

	if (IsDesignTime()) return true;

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);

	PlayersChangedListenerHandle = MessageSubsystem.RegisterListener<FHeistLobbyPlayersChangedMessage>(
		HeistMessageTags::Message_Lobby_PlayersChanged,
		[this](FGameplayTag Channel, const FHeistLobbyPlayersChangedMessage& Message)
		{
			OnPlayersChangedMessageReceived(Channel, Message);
		});

	ReadyStateChangedListenerHandle = MessageSubsystem.RegisterListener<FHeistLobbyReadyStateChangedMessage>(
		HeistMessageTags::Message_Lobby_ReadyStateChanged,
		[this](FGameplayTag Channel, const FHeistLobbyReadyStateChangedMessage& Message)
		{
			OnReadyStateChangedMessageReceived(Channel, Message);
		});

	InviteCodeChangedListenerHandle = MessageSubsystem.RegisterListener<FHeistLobbyInviteCodeChangedMessage>(
		HeistMessageTags::Message_Lobby_InviteCodeChanged,
		[this](FGameplayTag Channel, const FHeistLobbyInviteCodeChangedMessage& Message)
		{
			OnInviteCodeChangedMessageReceived(Channel, Message);
		});

	if (ButtonStartGame)
	{
		ButtonStartGame->SetIsEnabled(false);
		ButtonStartGame->OnClicked.AddDynamic(this, &ThisClass::OnButtonStartGameClicked);
	}

	if (ButtonReady)
	{
		ButtonReady->OnClicked.AddDynamic(this, &ThisClass::OnButtonReadyClicked);
	}

	if (ButtonCopyInviteCode)
	{
		ButtonCopyInviteCode->OnClicked.AddDynamic(this, &ThisClass::OnButtonCopyInviteCodeClicked);
	}

	return true;
}

void UHeistLobbyWidget::NativeDestruct()
{
	PlayersChangedListenerHandle.Unregister();
	ReadyStateChangedListenerHandle.Unregister();
	InviteCodeChangedListenerHandle.Unregister();

	Super::NativeDestruct();
}

void UHeistLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshPlayerList();

	const bool bIsHost = IsValid(GetWorld()->GetAuthGameMode());

	if (ButtonStartGame)
	{
		ButtonStartGame->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (ButtonReady)
	{
		ButtonReady->SetVisibility(bIsHost ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	// 위젯 생성 시점에 이미 복제된 초대 코드가 있으면 즉시 표시.
	// 아직 없는 경우 OnInviteCodeChangedMessageReceived에서 처리됨.
	AHeistLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AHeistLobbyGameState>();
	if (IsValid(LobbyGameState) && !LobbyGameState->GetInviteCode().IsEmpty())
	{
		RefreshInviteCode(LobbyGameState->GetInviteCode());
	}
}

void UHeistLobbyWidget::OnPlayersChangedMessageReceived(FGameplayTag Channel, const FHeistLobbyPlayersChangedMessage& Message)
{
	RefreshPlayerList();
	RefreshStartButtonState();
}

void UHeistLobbyWidget::OnReadyStateChangedMessageReceived(FGameplayTag Channel, const FHeistLobbyReadyStateChangedMessage& Message)
{
	RefreshPlayerList();
	RefreshStartButtonState();
}

void UHeistLobbyWidget::OnInviteCodeChangedMessageReceived(FGameplayTag Channel, const FHeistLobbyInviteCodeChangedMessage& Message)
{
	RefreshInviteCode(Message.InviteCode);
}

void UHeistLobbyWidget::RefreshInviteCode(const FString& NewInviteCode)
{
	CurrentInviteCode = NewInviteCode;

	if (TextBlockInviteCode)
	{
		TextBlockInviteCode->SetText(FText::FromString(NewInviteCode));
	}
}

void UHeistLobbyWidget::RefreshStartButtonState()
{
	if (!ButtonStartGame) return;

	AHeistLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AHeistLobbyGameState>();
	const bool bAllReady = IsValid(LobbyGameState) && LobbyGameState->AreAllPlayersReady();

	ButtonStartGame->SetIsEnabled(bAllReady);
}

void UHeistLobbyWidget::RefreshPlayerList()
{
	if (!ScrollBoxPlayers) return;

	ScrollBoxPlayers->ClearChildren();

	AGameStateBase* CurrentGameState = GetWorld()->GetGameState();
	if (!IsValid(CurrentGameState)) return;

	for (APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		const AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(PlayerState);
		if (!IsValid(HeistPS)) continue;

		const FString HostPrefix = HeistPS->GetIsHost() ? TEXT("[방장] ") : TEXT("");
		const FString ReadySuffix = HeistPS->GetIsReady() ? TEXT(" [준비완료]") : TEXT(" [대기중]");
		const FString EntryText = HostPrefix + HeistPS->GetPlayerName() + ReadySuffix;

		UTextBlock* PlayerEntry = NewObject<UTextBlock>(this);
		PlayerEntry->SetText(FText::FromString(EntryText));
		ScrollBoxPlayers->AddChild(PlayerEntry);
	}
}

void UHeistLobbyWidget::OnButtonStartGameClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!IsValid(PC)) return;

	if (AHeistLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AHeistLobbyGameMode>())
	{
		LobbyGameMode->RequestStartGame(PC);
	}
}

void UHeistLobbyWidget::OnButtonReadyClicked()
{
	AHeistPlayerController* HeistPC = GetOwningPlayer<AHeistPlayerController>();
	if (!IsValid(HeistPC)) return;

	const AHeistPlayerState* HeistPS = HeistPC->GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS)) return;

	HeistPC->ServerRequestSetReady(!HeistPS->GetIsReady());
}

void UHeistLobbyWidget::OnButtonCopyInviteCodeClicked()
{
	if (CurrentInviteCode.IsEmpty()) return;

	FPlatformApplicationMisc::ClipboardCopy(*CurrentInviteCode);
}
