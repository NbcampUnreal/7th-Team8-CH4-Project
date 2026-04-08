#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DebugUISettings.generated.h"

/**
 * 
 */
UCLASS(Config = Game)
class DEBUGUI_API UDebugUISettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UDebugUISettings() { CategoryName = TEXT("DebugPlugin"); }

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "DebugUI")
	TSoftClassPtr<UUserWidget> DebugWidgetClass;
};
