
#include "HeistUIInputModeLibrary.h"

  #include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"

  void UHeistUIInputModeLibrary::ApplyInputMode(APlayerController* PlayerController, EHeistUIInputMode Mode, UWidget* FocusWidget)
  {
  	if (!IsValid(PlayerController))
  	{
  		return;
  	}

  	switch (Mode)
  	{
  	case EHeistUIInputMode::Lobby:
  		{
  			UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(
									  PlayerController,
									  FocusWidget,
									  EMouseLockMode::DoNotLock,
									  false
							  );
  			PlayerController->SetShowMouseCursor(true);
  			break;
  		}

  	case EHeistUIInputMode::Briefing:
  		{
  			UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
									  PlayerController,
									  FocusWidget,
									  EMouseLockMode::DoNotLock,
									  false
							  );
  			PlayerController->SetShowMouseCursor(true);
  			break;
  		}

  	case EHeistUIInputMode::Match:
  		{
  			UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(
					  PlayerController,
					  FocusWidget,
					  EMouseLockMode::DoNotLock,
					  false
			  );
  			PlayerController->SetShowMouseCursor(true);
  			break;
  		}

  	default:
  		break;
  	}
  }
