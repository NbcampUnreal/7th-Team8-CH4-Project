#include "HeistBriefingUISubsystem.h"

#include "HeistBriefingWidgetInterface.h"
#include "HeistUIInputModeLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

void UHeistBriefingUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UHeistMessageSubsystem& MS = UHeistMessageSubsystem::Get(this);

	ContextReadyHandle = MS.RegisterListener<FHeistBriefingContextReadyMessage>(
		HeistMessageTags::Message_Briefing_ContextReady,
		[this](FGameplayTag Channel, const FHeistBriefingContextReadyMessage& Msg)
		{
			HandleBriefingContextReady(Channel, Msg);
		});

	EndHandle = MS.RegisterListener<FHeistBriefingEndMessage>(
		HeistMessageTags::Message_Briefing_End,
		[this](FGameplayTag Channel, const FHeistBriefingEndMessage& Msg)
		{
			HandleBriefingEnd(Channel, Msg);
		});
}

void UHeistBriefingUISubsystem::Deinitialize()
{
	ContextReadyHandle.Unregister();
	EndHandle.Unregister();

	Super::Deinitialize();
}

void UHeistBriefingUISubsystem::HandleBriefingContextReady(
	FGameplayTag, const FHeistBriefingContextReadyMessage& Msg)
{
	if (!IsValid(Msg.WidgetClass)) return;
	if (!Msg.WidgetClass->IsChildOf(UUserWidget::StaticClass())) return;

	APlayerController* PC = GetLocalPlayer()->GetPlayerController(GetWorld());
	if (!IsValid(PC)) return;

	if (!IsValid(BriefingWidgetInstance))
	{
		BriefingWidgetInstance = CreateWidget<UUserWidget>(PC, Msg.WidgetClass);
		if (!IsValid(BriefingWidgetInstance)) return;
		BriefingWidgetInstance->AddToViewport(10);

		UHeistUIInputModeLibrary::ApplyInputMode(
		  PC,
		  EHeistUIInputMode::Briefing,
		  BriefingWidgetInstance);
	}

	if (IHeistBriefingWidgetInterface* BriefingWidgetInterface = Cast<IHeistBriefingWidgetInterface>(BriefingWidgetInstance))
	{
		BriefingWidgetInterface->InitializeForBriefing(
			Msg.BriefingPlayerComponent,
			Msg.DrawingSyncComponent,
			Msg.ViewMode);
	}
}

void UHeistBriefingUISubsystem::HandleBriefingEnd(FGameplayTag, const FHeistBriefingEndMessage&)
{
	APlayerController* PC = GetLocalPlayer()->GetPlayerController(GetWorld());

	if (IsValid(BriefingWidgetInstance))
	{
		BriefingWidgetInstance->RemoveFromParent();
		BriefingWidgetInstance = nullptr;

		UHeistUIInputModeLibrary::ApplyInputMode(
			   PC,
			   EHeistUIInputMode::Match,
			   nullptr);
	}
}
