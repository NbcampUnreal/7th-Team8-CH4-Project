
#include "HeistBriefingScreenWidget.h"

#include "Core/HeistMatchGameState.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "Core/HeistBriefingPointDataAsset.h"
#include "HeistBriefingMapWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HeistBriefingPlayerComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UHeistBriefingPointButtonBinding::Initialize(UHeistBriefingScreenWidget* InOwner, const FHeistSpawnPointData& InPointData)
{
	Owner = InOwner;
	PointData = InPointData;
}

void UHeistBriefingPointButtonBinding::HandleClicked()
{
	if (IsValid(Owner))
	{
		Owner->HandleBriefingPointButtonClicked(PointData);
	}
}

void UHeistBriefingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(MapWidget))
	{
		MapWidget->OnBriefingMapInitialized.AddDynamic(this, &ThisClass::HandleMapInitialized);
	}
}

void UHeistBriefingScreenWidget::NativeDestruct()
{
	if (IsValid(MapWidget))
	{
		MapWidget->OnBriefingMapInitialized.RemoveDynamic(this, &ThisClass::HandleMapInitialized);
	}

	if (IsValid(CachedBriefingPlayerComponent))
	{
		CachedBriefingPlayerComponent->OnThiefSelectionCountsReceived.Remove(SelectionCountsHandle);
		CachedBriefingPlayerComponent->OnPoliceObjectiveSelectionReceived.RemoveAll(this);
	}

	PhaseChangedHandle.Unregister();

	ClearBriefingPointButtons();

	Super::NativeDestruct();
}

void UHeistBriefingScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(Text_Timer))
	{
		return;
	}

	const AHeistMatchGameState* HeistGS = GetWorld()
		? GetWorld()->GetGameState<AHeistMatchGameState>()
		: nullptr;

	if (!IsValid(HeistGS) || !HeistGS->IsBriefingPhase() || HeistGS->GetPhaseEndServerTime() <= 0.f)
	{
		Text_Timer->SetText(FText::FromString(TEXT("00:00")));
		return;
	}

	const float RemainingTime = FMath::Max(
		0.f,
		HeistGS->GetPhaseEndServerTime() - HeistGS->GetServerWorldTimeSeconds());
	const int32 RemainingSeconds = FMath::CeilToInt(RemainingTime);
	const int32 Minutes = RemainingSeconds / 60;
	const int32 Seconds = RemainingSeconds % 60;

	Text_Timer->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
}

void UHeistBriefingScreenWidget::InitializeForBriefing(
	UHeistBriefingPlayerComponent* InBriefingPlayerComponent,
	UHeistBriefingDrawingSyncComponent* InDrawingSyncComponent,
	EHeistBriefingViewMode InViewMode)
{
	if (IsValid(CachedBriefingPlayerComponent))
	{
		CachedBriefingPlayerComponent->OnPoliceObjectiveSelectionReceived.RemoveAll(this);
	}

	CachedBriefingPlayerComponent = InBriefingPlayerComponent;
	CurrentViewMode = InViewMode;
	bMapInitialized = false;

	// 잠금 상태 초기화 및 리스너 등록
	PhaseChangedHandle.Unregister();
	PhaseChangedHandle = UHeistMessageSubsystem::Get(this).RegisterListener<FHeistPhaseChangedMessage>(
		HeistMessageTags::Message_Phase_Changed,
		[this](FGameplayTag, const FHeistPhaseChangedMessage& Msg)
		{
			HandlePhaseChanged(Msg);
		});

	if (const AHeistMatchGameState* HeistGS = GetWorld() ? GetWorld()->GetGameState<AHeistMatchGameState>() : nullptr)
	{
		bSelectionLocked = HeistGS->IsBriefingSelectionLocked();
	}

	if (!IsValid(MapWidget))
	{
		return;
	}

	MapWidget->InitializeForBriefing(
		InBriefingPlayerComponent,
		InDrawingSyncComponent,
		InViewMode);

	if (CurrentViewMode == EHeistBriefingViewMode::Police && IsValid(InBriefingPlayerComponent))
	{
		InBriefingPlayerComponent->OnPoliceObjectiveSelectionReceived.AddUObject(
			this, &ThisClass::HandlePoliceObjectiveSelectionReceived);
	}

	if (CurrentViewMode == EHeistBriefingViewMode::Thief && IsValid(InBriefingPlayerComponent))
	{
		SelectionCountsHandle = InBriefingPlayerComponent->OnThiefSelectionCountsReceived
			.AddUObject(this, &ThisClass::HandleThiefSelectionCounts);

		InBriefingPlayerComponent->ServerRequestThiefCounts();
	}

	RebuildSelectionList();
}

void UHeistBriefingScreenWidget::SelectBriefingPointByData(const FHeistSpawnPointData& InPointData)
{
	if (bSelectionLocked || !IsValid(CachedBriefingPlayerComponent))
	{
		return;
	}

	switch (InPointData.PointType)
	{
	case EHeistBriefingPointType::ThiefInsertion:
		CachedBriefingPlayerComponent->ServerSetThiefSpawnPoint(InPointData.Key);
		break;

	case EHeistBriefingPointType::PoliceObjective:
		CachedBriefingPlayerComponent->ServerSetPoliceObjectiveKey(InPointData.Key);
		HandlePoliceLocalSelection(InPointData.Key);
		break;

	default:
		break;
	}
}

void UHeistBriefingScreenWidget::GetVisibleBriefingPoints(TArray<FHeistSpawnPointData>& OutVisiblePoints) const
{
	OutVisiblePoints.Reset();
	const FName ActiveLayerId = IsValid(MapWidget) ? MapWidget->GetActiveLayerId() : NAME_None;

	if (!IsValid(BriefingPointData)) return;

	for (const FHeistSpawnPointData& PointData : BriefingPointData->Points)
	{
		if (PointData.VisibleTo != CurrentViewMode)
		{
			continue;
		}

		if (ActiveLayerId != NAME_None
			&& PointData.LayerId != NAME_None
			&& PointData.LayerId != ActiveLayerId)
		{
			continue;
		}

		OutVisiblePoints.Add(PointData);
	}
}

void UHeistBriefingScreenWidget::RefreshBriefingPointButtons()
{
	NotifyVisiblePointsChanged();
}

void UHeistBriefingScreenWidget::HandleMapInitialized(UHeistBriefingPlayerComponent* InBriefingPlayerComponent)
{
	CachedBriefingPlayerComponent = InBriefingPlayerComponent;
	bMapInitialized = true;
	NotifyVisiblePointsChanged();
}

void UHeistBriefingScreenWidget::NotifyVisiblePointsChanged()
{
	if (!bMapInitialized)
	{
		return;
	}

	TArray<FHeistSpawnPointData> VisiblePoints;
	GetVisibleBriefingPoints(VisiblePoints);
	RebuildBriefingPointButtonsInternal(VisiblePoints);
}

void UHeistBriefingScreenWidget::RebuildBriefingPointButtonsInternal(const TArray<FHeistSpawnPointData>& VisiblePoints)
{
	ClearBriefingPointButtons();

	if (!IsValid(PointButtonCanvas) || !IsValid(SpawnPointButtonClass))
	{
		return;
	}

	for (const FHeistSpawnPointData& PointData : VisiblePoints)
	{
		UUserWidget* ButtonWidget = CreateWidget<UUserWidget>(this, SpawnPointButtonClass);
		if (!IsValid(ButtonWidget))
		{
			continue;
		}

		if (UCanvasPanelSlot* CanvasSlot = PointButtonCanvas->AddChildToCanvas(ButtonWidget))
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAnchors(FAnchors(PointData.NormalizedPosition.X, PointData.NormalizedPosition.Y));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetPosition(FVector2D::ZeroVector);
		}

		if (UTextBlock* LabelText = Cast<UTextBlock>(ButtonWidget->WidgetTree->FindWidget(TEXT("Text_DisplayName"))))
		{
			LabelText->SetText(PointData.DisplayName);
		}

		if (UButton* SelectButton = Cast<UButton>(ButtonWidget->WidgetTree->FindWidget(TEXT("Button_Select"))))
		{
			UHeistBriefingPointButtonBinding* Binding = NewObject<UHeistBriefingPointButtonBinding>(this);
			Binding->Initialize(this, PointData);
			SelectButton->OnClicked.AddDynamic(Binding, &UHeistBriefingPointButtonBinding::HandleClicked);
			PointButtonBindings.Add(Binding);
		}

		ConfigureBriefingPointButton(ButtonWidget, PointData);
		SpawnedPointButtons.Add(ButtonWidget);
	}

	// 재빌드 후 현재 잠금 상태 반영
	if (bSelectionLocked)
	{
		SetBriefingPointButtonsEnabled(false);
	}
}

void UHeistBriefingScreenWidget::ClearBriefingPointButtons()
{
	for (UUserWidget* ButtonWidget : SpawnedPointButtons)
	{
		if (IsValid(ButtonWidget))
		{
			ButtonWidget->RemoveFromParent();
		}
	}

	SpawnedPointButtons.Reset();
	PointButtonBindings.Reset();
}

void UHeistBriefingScreenWidget::HandleBriefingPointButtonClicked(const FHeistSpawnPointData& PointData)
{
	SelectBriefingPointByData(PointData);
}

void UHeistBriefingScreenWidget::HandlePhaseChanged(const FHeistPhaseChangedMessage& Message)
{
	bSelectionLocked = Message.bBriefingSelectionLocked;
	SetBriefingPointButtonsEnabled(!bSelectionLocked);
}

void UHeistBriefingScreenWidget::SetBriefingPointButtonsEnabled(bool bEnabled)
{
	for (UUserWidget* ButtonWidget : SpawnedPointButtons)
	{
		if (IsValid(ButtonWidget))
		{
			ButtonWidget->SetIsEnabled(bEnabled);
		}
	}
}

void UHeistBriefingScreenWidget::RebuildSelectionList()
{
	SelectionListBox->ClearChildren();
	SelectionRowMap.Reset();

	if (!IsValid(BriefingPointData) || !IsValid(SelectionRowClass)) return;

	for (const FHeistSpawnPointData& Point : BriefingPointData->Points)
	{
		if (Point.VisibleTo != CurrentViewMode) continue;

		UUserWidget* Row = CreateWidget<UUserWidget>(this, SelectionRowClass);
		if (!IsValid(Row)) continue;

		if (UTextBlock* Name = Cast<UTextBlock>(Row->WidgetTree->FindWidget(TEXT("Text_PointName"))))
			Name->SetText(Point.DisplayName);

		if (UTextBlock* Count = Cast<UTextBlock>(Row->WidgetTree->FindWidget(TEXT("Text_Count"))))
			Count->SetText(FText::GetEmpty());

		SelectionListBox->AddChild(Row);
		SelectionRowMap.Add(Point.Key, Row);
	}

	if (CurrentViewMode == EHeistBriefingViewMode::Police && CachedPoliceSelectedKey != NAME_None)
	{
		HandlePoliceLocalSelection(CachedPoliceSelectedKey);
	}
}

void UHeistBriefingScreenWidget::HandlePoliceLocalSelection(FName SelectedKey)
{
	CachedPoliceSelectedKey = SelectedKey;

	for (auto& Pair : SelectionRowMap)
	{
		if (UTextBlock* Name = Cast<UTextBlock>(
			Pair.Value->WidgetTree->FindWidget(TEXT("Text_PointName"))))
		{
			const bool bSelected = Pair.Key == SelectedKey;
			Name->SetColorAndOpacity(bSelected
				? FSlateColor(FLinearColor::Yellow)  // 노랑
				: FSlateColor(FLinearColor::White));
		}
	}
}

void UHeistBriefingScreenWidget::HandlePoliceObjectiveSelectionReceived(FName SelectedKey)
{
	CachedPoliceSelectedKey = SelectedKey;
	HandlePoliceLocalSelection(SelectedKey);
}

void UHeistBriefingScreenWidget::HandleThiefSelectionCounts(
	const TArray<FHeistBriefingSelectionCount>& Counts)
{
	for (auto& Pair : SelectionRowMap)
	{
		const FHeistBriefingSelectionCount* Found = Counts.FindByPredicate(
			[&](const FHeistBriefingSelectionCount& C){ return C.Key == Pair.Key; });

		if (UTextBlock* CountText = Cast<UTextBlock>(
			Pair.Value->WidgetTree->FindWidget(TEXT("Text_Count"))))
		{
			const int32 N = Found ? Found->Count : 0;
			CountText->SetText(N > 0 ? FText::AsNumber(N) : FText::GetEmpty());
		}
	}
}
