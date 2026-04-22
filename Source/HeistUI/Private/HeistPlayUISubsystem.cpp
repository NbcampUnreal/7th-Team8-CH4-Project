#include "HeistPlayUISubsystem.h"
#include "HeistPlayHUD.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "Blueprint/UserWidget.h"

void UHeistPlayUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UHeistMessageSubsystem& MS = UHeistMessageSubsystem::Get(this);

	ReadyPhaseHandle = MS.RegisterListener<FHeistPlayHUDReadyMessage>(
		HeistMessageTags::Message_PlayHUD_Ready,
		[this](FGameplayTag, const FHeistPlayHUDReadyMessage& Msg)
		{
			ShowPlayHUD(Msg.PlayHUDClass);
		});
	EndHandle = MS.RegisterListener<FHeistPhaseChangedMessage>(
		HeistMessageTags::Message_Phase_Changed,
		[this](FGameplayTag, const FHeistPhaseChangedMessage& Msg)
		{
			if (Msg.CurrentPhase == EHeistMatchPhase::Result)
			{
				HidePlayHUD();
			}
		});
}

void UHeistPlayUISubsystem::Deinitialize()
{
	HidePlayHUD();
	ReadyPhaseHandle.Unregister();
	EndHandle.Unregister();
	Super::Deinitialize();
}

void UHeistPlayUISubsystem::ShowPlayHUD(TSubclassOf<UObject> InWidgetClass)
{
	if (!IsValid(InWidgetClass)) return;

	if (IsValid(PlayHUDInstance))
	{
		PlayHUDInstance->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	APlayerController* PC = GetLocalPlayer()->GetPlayerController(World);

	PlayHUDInstance = CreateWidget<UHeistPlayHUD>(PC, TSubclassOf<UHeistPlayHUD>(InWidgetClass));
	if (!IsValid(PlayHUDInstance)) return;

	PlayHUDInstance->AddToViewport();
}

void UHeistPlayUISubsystem::HidePlayHUD()
{
	if (!IsValid(PlayHUDInstance)) return;

	PlayHUDInstance->RemoveFromParent();
	PlayHUDInstance = nullptr;
}
