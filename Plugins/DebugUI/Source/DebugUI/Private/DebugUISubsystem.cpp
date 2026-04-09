#include "DebugUISubsystem.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Runtime/UMG/Public/UMG.h"
#include "DebugUISettings.h"


void UDebugUISubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);

	if (NewPlayerController && NewPlayerController->IsLocalController()) 
	{ 
		if (NewPlayerController->InputComponent) 
		{
			NewPlayerController->InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &UDebugUISubsystem::ToggleDebugWidget);
		}
	}
}

//ProjectSettings에서(가장 하단에 추가됨) Widget연결(WBP_...)이 되어 있어야 동작함
void UDebugUISubsystem::ToggleDebugWidget()
{

	if (DebugWidgetInstance == nullptr) 
	{
		const UDebugUISettings* Settings = GetDefault<UDebugUISettings>();
		if (Settings && !Settings->DebugWidgetClass.IsNull()) 
		{
			// 소프트 클래스를 동기식으로 로드 (최초 1회)
			UClass* LoadedClass = Settings->DebugWidgetClass.LoadSynchronous();

			UWorld* World = GetWorld();
			APlayerController* PC = GetLocalPlayer()->GetPlayerController(World);
			if (World && PC)
			{
				DebugWidgetInstance = CreateWidget<UUserWidget>(World, LoadedClass);
			}
		}
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
