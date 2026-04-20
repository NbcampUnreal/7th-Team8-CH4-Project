
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "HeistUIInputModePolicy.h"
#include "HeistUIInputModeLibrary.generated.h"

class APlayerController;
class UWidget;

UCLASS()
class HEISTUI_API UHeistUIInputModeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HeistUI|Input")
	static void ApplyInputMode(APlayerController* PlayerController, EHeistUIInputMode Mode, UWidget* FocusWidget);
};
