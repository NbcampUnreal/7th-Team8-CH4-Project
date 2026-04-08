#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "DebugUISubsystem.generated.h"


UCLASS()
class DEBUGUI_API UDebugUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
	
public:

	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	UFUNCTION(BlueprintCallable, Category = "DebugUI")
	void ToggleDebugWidget();
private:
	UPROPERTY()
	TSubclassOf<class UUserWidget> DebugWidgetClass;

	UPROPERTY()
	class UUserWidget* DebugWidgetInstance;

	void CheckDebugInput();

	FTimerHandle InputCheckTimer;
};
