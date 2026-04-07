#include "DebugUISubsystem.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Runtime/UMG/Public/UMG.h"

UDebugUISubsystem::UDebugUISubsystem()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> WBPClass(TEXT("/DebugUI/HUD/WBP_DebugMain.WBP_DebugMain_C"));
	if (WBPClass.Succeeded())
	{
		DebugWidgetClass = WBPClass.Class; 
	}
}

void UDebugUISubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);

	if (NewPlayerController && NewPlayerController->IsLocalController()) 
	{ 
		if (!GetWorld()->GetTimerManager().IsTimerActive(InputCheckTimer)) 
		{ 
			GetWorld()->GetTimerManager().SetTimer(InputCheckTimer, this, &UDebugUISubsystem::CheckDebugInput, 0.01f, true); 
		}
	}
}

void UDebugUISubsystem::ToggleDebugWidget()
{
	if (!DebugWidgetClass) return; 

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = GetLocalPlayer()->GetPlayerController(World);
	if (!PC) return;

	if (!DebugWidgetInstance) 
	{ 
		DebugWidgetInstance = CreateWidget<UUserWidget>(World, DebugWidgetClass);
	} 

	if (DebugWidgetInstance) 
	{ 
		if (DebugWidgetInstance->IsInViewport()) 
		{ 
			DebugWidgetInstance->RemoveFromParent();
		} 
		else 
		{ 
			DebugWidgetInstance->AddToViewport(9999); 
		}
	}
}

void UDebugUISubsystem::CheckDebugInput()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer) return; 
	
	APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()); 
	
	if (PC && PC->WasInputKeyJustPressed(EKeys::F6)) 
	{
		ToggleDebugWidget();
	}
}
