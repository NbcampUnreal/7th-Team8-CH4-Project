#include "HeistSystemMenuWidget.h"

#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "MultiplayerSessionsSubsystem.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

bool UHeistSystemMenuWidget::Initialize()
{
	if (!Super::Initialize()) return false;

	if (IsDesignTime()) return true;

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);
	ToggleListenerHandle = MessageSubsystem.RegisterListener<FHeistSystemMenuToggleMessage>(
		HeistMessageTags::Message_UI_SystemMenuToggle,
		[this](FGameplayTag Channel, const FHeistSystemMenuToggleMessage& Message)
		{
			OnToggleMessageReceived(Channel, Message);
		});

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		SessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (ButtonResume)
	{
		ButtonResume->OnClicked.AddDynamic(this, &ThisClass::OnButtonResumeClicked);
	}
	if (ButtonSettings)
	{
		ButtonSettings->OnClicked.AddDynamic(this, &ThisClass::OnButtonSettingsClicked);
	}
	if (ButtonLeave)
	{
		ButtonLeave->OnClicked.AddDynamic(this, &ThisClass::OnButtonLeaveClicked);
	}
	if (ButtonQuit)
	{
		ButtonQuit->OnClicked.AddDynamic(this, &ThisClass::OnButtonQuitClicked);
	}

	return true;
}

void UHeistSystemMenuWidget::NativeDestruct()
{
	ToggleListenerHandle.Unregister();

	if (IsValid(SessionsSubsystem))
	{
		SessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(
			this, &ThisClass::OnDestroySessionComplete);
	}

	Super::NativeDestruct();
}

void UHeistSystemMenuWidget::OnToggleMessageReceived(FGameplayTag Channel, const FHeistSystemMenuToggleMessage& Message)
{
	const bool bCurrentlyVisible = GetVisibility() == ESlateVisibility::Visible;
	SetMenuVisibility(!bCurrentlyVisible);
}

void UHeistSystemMenuWidget::SetMenuVisibility(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	APlayerController* PC = GetOwningPlayer();
	if (!IsValid(PC)) return;

	if (bVisible)
	{
		PC->SetInputMode(FInputModeUIOnly());
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void UHeistSystemMenuWidget::OnButtonResumeClicked()
{
	SetMenuVisibility(false);
}

void UHeistSystemMenuWidget::OnButtonSettingsClicked()
{
	OnSettingsRequested();
}

void UHeistSystemMenuWidget::OnButtonLeaveClicked()
{
	if (!IsValid(SessionsSubsystem)) return;

	if (ButtonLeave)
	{
		ButtonLeave->SetIsEnabled(false);
	}
	if (ButtonQuit)
	{
		ButtonQuit->SetIsEnabled(false);
	}

	SessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(
		this, &ThisClass::OnDestroySessionComplete);
	SessionsSubsystem->DestroySession();
}

void UHeistSystemMenuWidget::OnButtonQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UHeistSystemMenuWidget::OnDestroySessionComplete(bool bWasSuccessful)
{
	SessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(
		this, &ThisClass::OnDestroySessionComplete);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->ClientTravel(MainMenuPath, ETravelType::TRAVEL_Absolute);
	}
}
