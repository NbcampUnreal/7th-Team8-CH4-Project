#include "HeistPlayHUD.h"
#include "HeistSystemMessageWidget.h"
#include "HeistThiefSlotSetWidget.h"
#include "HeistThiefSlotWidget.h"

#include "Core/HeistMatchGameState.h"
#include "Core/HeistPlayerState.h"
#include "Character/HeistTags_State.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "EngineUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"

void UHeistPlayHUD::NativeConstruct()
{
	Super::NativeConstruct();

	static const FName QuotaNames[] = {
		TEXT("WBP_QuotaSegment_A"),
		TEXT("WBP_QuotaSegment_B"),
		TEXT("WBP_QuotaSegment_C")
	};

	if (UUserWidget* TimerWidget = Cast<UUserWidget>(WidgetTree->FindWidget(TEXT("WBP_PlayTimer"))))
	{
		PlayTimerText = Cast<UTextBlock>(TimerWidget->WidgetTree->FindWidget(TEXT("TextBlock_PlayTimer")));
	}

	//Overlay_Thief_Only = Cast<UOverlay>(WidgetTree->FindWidget(TEXT("Overlay_Thief_Only")));
	//Overlay_Police_Only = Cast<UOverlay>(WidgetTree->FindWidget(TEXT("Overlay_Police_Only")));

	QuotaBars.Reset();
	for (const FName& QuotaName : QuotaNames)
	{
		if (UUserWidget* QuotaWidget = Cast<UUserWidget>(WidgetTree->FindWidget(QuotaName)))
		{
			if (UProgressBar* ProgressBar = Cast<UProgressBar>(QuotaWidget->WidgetTree->FindWidget(TEXT("ProgressBar_30"))))
			{
				QuotaBars.Add(ProgressBar);
			}
		}
	}

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);

	PhaseTimeHandle = MessageSubsystem.RegisterListener<FHeistPhaseTimeUpdatedMessage>(
		HeistMessageTags::Message_Phase_TimeUpdated,
		[this](FGameplayTag, const FHeistPhaseTimeUpdatedMessage& Msg)
		{
			const AHeistMatchGameState* MatchGS = GetWorld() ? GetWorld()->GetGameState<AHeistMatchGameState>() : nullptr;
			UpdateTimerText(IsValid(MatchGS) && MatchGS->IsExecutionPhase() ? Msg.RemainingTime : 0.f);
		});

	ZoneScoresHandle = MessageSubsystem.RegisterListener<FHeistZoneScoresUpdatedMessage>(
		HeistMessageTags::Message_PlayHUD_ZoneScoresUpdated,
		[this](FGameplayTag, const FHeistZoneScoresUpdatedMessage& Msg)
		{
			for (int32 Index = 0; Index < QuotaBars.Num(); ++Index)
			{
				if (!IsValid(QuotaBars[Index]))
				{
					continue;
				}

				const FZoneScoreData ZoneScore = Msg.ZoneScores.IsValidIndex(Index)
					? Msg.ZoneScores[Index]
					: FZoneScoreData();
				const float Percent = ZoneScore.TargetScore > 0.f
					? ZoneScore.CurrentScore / ZoneScore.TargetScore
					: 0.f;

				QuotaBars[Index]->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
			}
		});

	PoliceObjectiveHandle = MessageSubsystem.RegisterListener<FHeistPoliceObjectiveUpdatedMessage>(
		HeistMessageTags::Message_PlayHUD_PoliceObjectiveUpdated,
		[this](FGameplayTag, const FHeistPoliceObjectiveUpdatedMessage& Msg)
		{
			if (IsValid(Text_ProtectObjective))
			{
				Text_ProtectObjective->SetText(FText::Format(
					NSLOCTEXT("HeistPlayHUD", "ProtectObjectiveFormat", "보호 - {0}"),
					Msg.DisplayName));
			}
		});

	PlayersChangedHandle = MessageSubsystem.RegisterListener<FHeistLobbyPlayersChangedMessage>(
		HeistMessageTags::Message_Lobby_PlayersChanged,
		[this](FGameplayTag, const FHeistLobbyPlayersChangedMessage& Msg)
		{
			RefreshThiefSlots();
		});

	ThiefStateChangedHandle = MessageSubsystem.RegisterListener<FHeistPlayHUDThiefStateChangedMessage>(
		HeistMessageTags::Message_PlayHUD_ThiefStateChanged,
		[this](FGameplayTag, const FHeistPlayHUDThiefStateChangedMessage& Msg)
		{
			HandleThiefStateChanged(Msg);
		});

	if (SystemMessageWidgetClass)
	{
		UHeistSystemMessageWidget* SystemMessageWidget = CreateWidget<UHeistSystemMessageWidget>(
			GetOwningPlayer(), SystemMessageWidgetClass);
		if (IsValid(SystemMessageWidget))
		{
			SystemMessageWidget->AddToViewport();
		}
	}

	UpdateTimerText(0.f);
	RefreshThiefSlots();

	if (const AHeistMatchGameState* MatchGS = GetWorld() ? GetWorld()->GetGameState<AHeistMatchGameState>() : nullptr)
	{
		UpdateTimerText(
			MatchGS->IsExecutionPhase()
				? FMath::Max(MatchGS->GetPhaseEndServerTime() - MatchGS->GetServerWorldTimeSeconds(), 0.f)
				: 0.f);

		for (int32 Index = 0; Index < QuotaBars.Num(); ++Index)
		{
			if (!IsValid(QuotaBars[Index]))
			{
				continue;
			}

			const FZoneScoreData ZoneScore = MatchGS->GetZoneScore(Index);
			const float Percent = ZoneScore.TargetScore > 0.f
				? ZoneScore.CurrentScore / ZoneScore.TargetScore
				: 0.f;

			QuotaBars[Index]->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
		}

		if (IsValid(Text_ProtectObjective))
		{
			Text_ProtectObjective->SetText(FText::Format(
				NSLOCTEXT("HeistPlayHUD", "ProtectObjectiveFormat", "보호 - {0}"),
				MatchGS->GetPoliceObjectiveDisplayName()));
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CheckTimerHandle, this, &UHeistPlayHUD::InitializeIfBriefingPhase, 1.f, true);
	}
}

void UHeistPlayHUD::NativeDestruct()
{
	PhaseTimeHandle.Unregister();
	ZoneScoresHandle.Unregister();
	PoliceObjectiveHandle.Unregister();
	PlayersChangedHandle.Unregister();
	ThiefStateChangedHandle.Unregister();

	for (UHeistThiefSlotSetWidget* ThiefSlot : ThiefSlots)
	{
		if (IsValid(ThiefSlot))
		{
			ThiefSlot->Cleanup();
		}
	}
	ThiefSlots.Empty();
	ThiefSlotsByPlayerName.Empty();

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		Super::NativeDestruct();
		return;
	}

	if (World->GetTimerManager().IsTimerActive(CheckTimerHandle))
	{
		World->GetTimerManager().ClearTimer(CheckTimerHandle);
	}

	Super::NativeDestruct();
}

void UHeistPlayHUD::InitializeIfBriefingPhase()
{
	if (!IsValid(this)) return;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	APlayerState* MyPS = GetOwningPlayerState();
	AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(MyPS);
	if (!IsValid(HeistPS) || !IsValid(Thief_Police_Switcher)) return;

	bool bTeamReady = false;
	if (HeistPS->IsThief())
	{
		Thief_Police_Switcher->SetActiveWidgetIndex(1);
		if (IsValid(Overlay_Thief_Only))
		{
			Overlay_Thief_Only->SetVisibility(ESlateVisibility::Visible);
		}
		if (IsValid(Overlay_Police_Only))
		{
			Overlay_Police_Only->SetVisibility(ESlateVisibility::Collapsed);
		}
		bTeamReady = true;
	}
	else if (HeistPS->IsPolice())
	{
		Thief_Police_Switcher->SetActiveWidgetIndex(0);
		if (IsValid(Overlay_Police_Only))
		{
			Overlay_Police_Only->SetVisibility(ESlateVisibility::Visible);
		}
		if (IsValid(Overlay_Thief_Only))
		{
			Overlay_Thief_Only->SetVisibility(ESlateVisibility::Collapsed);
		}
		bTeamReady = true;
	}

	if (!bTeamReady) return;

	RefreshThiefSlots();

	if (const AHeistMatchGameState* MatchGS = World->GetGameState<AHeistMatchGameState>())
	{
		UpdateTimerText(
			MatchGS->IsExecutionPhase()
				? FMath::Max(MatchGS->GetPhaseEndServerTime() - MatchGS->GetServerWorldTimeSeconds(), 0.f)
				: 0.f);
	}

	if (World->GetTimerManager().IsTimerActive(CheckTimerHandle))
	{
		World->GetTimerManager().ClearTimer(CheckTimerHandle);
	}
}

void UHeistPlayHUD::UpdateTimerText(float RemainingTime)
{
	if (!IsValid(PlayTimerText))
	{
		return;
	}

	const int32 RemainingSeconds = FMath::CeilToInt(FMath::Max(RemainingTime, 0.f));
	const int32 Minutes = RemainingSeconds / 60;
	const int32 Seconds = RemainingSeconds % 60;

	PlayTimerText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
}

void UHeistPlayHUD::InitializeThiefSlots()
{
	RefreshThiefSlots();
}
void UHeistPlayHUD::RefreshThiefSlots()
{
	if (!IsValid(HBox_StatusGroup))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// 게임 스테이트에서 모든 플레이어 상태 조회
	AHeistMatchGameState* MatchGS = World->GetGameState<AHeistMatchGameState>();
	if (!IsValid(MatchGS))
	{
		return;
	}

	// 도둑 팀의 모든 플레이어 상태 수집
	TArray<AHeistPlayerState*> ThiefPlayerStates;
	for (TActorIterator<AHeistPlayerState> It(World); It; ++It)
	{
		AHeistPlayerState* PS = *It;
		if (IsValid(PS) && PS->IsThief())
		{
			ThiefPlayerStates.Add(PS);
		}
	}

	// VBox의 자식 위젯 중 ThiefSlotSetWidget 찾기
	TArray<UWidget*> SlotWidgets = HBox_StatusGroup->GetAllChildren();

	// 기존 슬롯 정리
	for (UHeistThiefSlotSetWidget* ThiefSlot : ThiefSlots)
	{
		if (IsValid(ThiefSlot))
		{
			ThiefSlot->Cleanup();
		}
	}
	ThiefSlots.Empty();
	ThiefSlotsByPlayerName.Empty();

	// 각 슬롯에 도둑 플레이어 할당
	for (int32 SlotIndex = 0; SlotIndex < SlotWidgets.Num(); ++SlotIndex)
	{
		UHeistThiefSlotSetWidget* SlotWidget = Cast<UHeistThiefSlotSetWidget>(SlotWidgets[SlotIndex]);
		if (!SlotWidget)
		{
			continue;
		}

		// 해당 인덱스의 도둑이 존재하면 할당, 없으면 빈 슬롯으로 표시
		AHeistPlayerState* PlayerStateToAssign = ThiefPlayerStates.IsValidIndex(SlotIndex)
			? ThiefPlayerStates[SlotIndex]
			: nullptr;

		SlotWidget->Initialize(PlayerStateToAssign);
		ThiefSlots.Add(SlotWidget);

		const FString& PlayerName = SlotWidget->GetCachedPlayerName();
		if (!PlayerName.IsEmpty())
		{
			ThiefSlotsByPlayerName.Add(PlayerName, SlotWidget);
		}
	}
}

void UHeistPlayHUD::HandleThiefStateChanged(const FHeistPlayHUDThiefStateChangedMessage& Message)
{
	AHeistPlayerState* HeistPlayerState = Cast<AHeistPlayerState>(Message.PlayerState.Get());
	UHeistThiefSlotSetWidget* TargetSlot = nullptr;

	if (!Message.PlayerName.IsEmpty())
	{
		if (TObjectPtr<UHeistThiefSlotSetWidget>* FoundSlot = ThiefSlotsByPlayerName.Find(Message.PlayerName))
		{
			TargetSlot = FoundSlot->Get();
		}
	}

	if (!IsValid(TargetSlot))
	{
		for (UHeistThiefSlotSetWidget* ThiefSlot : ThiefSlots)
		{
			if (!IsValid(ThiefSlot) || !ThiefSlot->MatchesPlayer(HeistPlayerState, Message.PlayerName))
			{
				continue;
			}

			TargetSlot = ThiefSlot;
			break;
		}
	}

	if (!IsValid(TargetSlot))
	{
		return;
	}

	if (!Message.PlayerName.IsEmpty())
	{
		TargetSlot->SetPlayerName(Message.PlayerName);
		ThiefSlotsByPlayerName.Add(Message.PlayerName, TargetSlot);
	}

	TargetSlot->UpdateThiefStateByName(Message.StateName);
}
