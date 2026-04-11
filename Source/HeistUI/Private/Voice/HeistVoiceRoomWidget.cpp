#include "Voice/HeistVoiceRoomWidget.h"
#include "Voice/HeistVoiceEntryWidget.h"

#include "Core/HeistPlayerState.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "Components/VerticalBox.h"
#include "GameFramework/GameStateBase.h"

void UHeistVoiceRoomWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Warning, TEXT("[VoiceRoom] NativeConstruct. EntryWidgetClass valid: %s"),
		IsValid(EntryWidgetClass) ? TEXT("YES") : TEXT("NO"));

	InitPlayerList();

	TalkingStateListenerHandle = UHeistMessageSubsystem::Get(this).RegisterListener<FHeistVoiceTalkingStateMessage>(
		HeistMessageTags::Message_Voice_TalkingStateChanged,
		[this](FGameplayTag Channel, const FHeistVoiceTalkingStateMessage& Message)
		{
			OnTalkingStateChanged(Channel, Message);
		});
}

void UHeistVoiceRoomWidget::NativeDestruct()
{
	Super::NativeDestruct();
	TalkingStateListenerHandle.Unregister();
}

void UHeistVoiceRoomWidget::InitPlayerList()
{
	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!IsValid(GameState)) return;
	if (!IsValid(EntryWidgetClass)) return;

	UE_LOG(LogTemp, Warning, TEXT("[VoiceRoom] InitPlayerList. PlayerArray count: %d"), GameState->PlayerArray.Num());

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (!IsValid(PlayerState) || EntryMap.Contains(PlayerState)) continue;

		UHeistVoiceEntryWidget* NewEntry = CreateWidget<UHeistVoiceEntryWidget>(GetOwningPlayer(), EntryWidgetClass);
		if (!IsValid(NewEntry)) continue;

		UE_LOG(LogTemp, Warning, TEXT("[VoiceRoom] Entry created for: %s"), *PlayerState->GetPlayerName());
		NewEntry->UpdateEntry(PlayerState->GetPlayerName(), false);
		PlayerList->AddChild(NewEntry);
		EntryMap.Add(PlayerState, NewEntry);
	}
}

void UHeistVoiceRoomWidget::OnTalkingStateChanged(FGameplayTag Channel, const FHeistVoiceTalkingStateMessage& Message)
{
	APlayerState* PlayerState = Message.PlayerState;
	UE_LOG(LogTemp, Warning, TEXT("[VoiceRoom] OnTalkingStateChanged. PlayerState valid: %s, bIsTalking: %s"),
		IsValid(PlayerState) ? TEXT("YES") : TEXT("NO"),
		Message.bIsTalking ? TEXT("TRUE") : TEXT("FALSE"));
	if (!IsValid(PlayerState)) return;

	// 아직 목록에 없는 플레이어면 추가 (늦게 접속한 경우)
	if (!EntryMap.Contains(PlayerState))
	{
		if (!IsValid(EntryWidgetClass)) return;

		UHeistVoiceEntryWidget* NewEntry = CreateWidget<UHeistVoiceEntryWidget>(GetOwningPlayer(), EntryWidgetClass);
		if (!IsValid(NewEntry)) return;

		PlayerList->AddChild(NewEntry);
		EntryMap.Add(PlayerState, NewEntry);
	}

	EntryMap[PlayerState]->UpdateEntry(PlayerState->GetPlayerName(), Message.bIsTalking);
}
