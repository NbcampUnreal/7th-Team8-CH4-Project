#include "HeistPlayHUD.h"
#include "HeistThiefSlotWidget.h"
#include "Core/HeistPlayerController.h"
#include "Core/HeistPlayerState.h"
#include "Core/HeistMatchGameState.h"

#include "Kismet/GameplayStatics.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"

void UHeistPlayHUD::NativeConstruct()
{
	Super::NativeConstruct();

	GetWorld()->GetTimerManager().SetTimer(CheckTimerHandle, this, &UHeistPlayHUD::InitializeIfBriefingPhase, 1.f, true);
}

void UHeistPlayHUD::NativeDestruct()
{
	if (GetWorld() || GetWorld()->GetTimerManager().IsTimerActive(CheckTimerHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);
	}

	Super::NativeDestruct();
}

void UHeistPlayHUD::InitializeIfBriefingPhase()
{
	if (!IsValid(this)) return;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	AGameStateBase* GS = World->GetGameState();
	AHeistMatchGameState* MatchGS = Cast<AHeistMatchGameState>(GS);
	if (MatchGS && MatchGS->IsBriefingPhase())
	{
		APlayerState* MyPS = GetOwningPlayerState();
		AHeistPlayerState* HeistPS = Cast<AHeistPlayerState>(MyPS);

		if (HeistPS) {
			if (HeistPS->IsThief()) {
				Thief_Police_Switcher->SetActiveWidgetIndex(1);
			}
			else if (HeistPS->IsPolice())
			{
				Thief_Police_Switcher->SetActiveWidgetIndex(0);
			}

			//InitializeThiefSlots();
		}

		if (World->GetTimerManager().IsTimerActive(CheckTimerHandle))
		{
			World->GetTimerManager().ClearTimer(CheckTimerHandle);
		}
	}
}

void UHeistPlayHUD::InitializeThiefSlots()
{
	// 현재 접속한 모든 PlayerState를 찾아 도둑만 리스트업
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHeistPlayerState::StaticClass(), FoundActors);

	TArray<UWidget*> Slots = VBox_ThiefStates->GetAllChildren();

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		UHeistThiefSlotWidget* SlotWidget = Cast<UHeistThiefSlotWidget>(Slots[i]);
		if (SlotWidget)
		{
			/*AHeistPlayerState* Target = FindThiefByIndex(i, FoundActors);
			SlotWidget->UpdateSlot(Target);*/
		}
	}

}
