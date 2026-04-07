#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DebugWidgetBase.generated.h"


UCLASS()
class DEBUGUI_API UDebugWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Debug|Network")
	FString GetNetModeString() const;
	UFUNCTION(BlueprintPure, Category = "Debug|Network")
	FString GetLocalRoleString() const;
	UFUNCTION(BlueprintPure, Category = "Debug|Network")
	float GetCurrentPing() const;
	UFUNCTION(BlueprintPure, Category = "Debug|System")
	float GetCurrentFPS() const;
protected:
	FString RoleToString(ENetRole Role) const;

	float DisplayFPS = 0.0f; 
	float TimerCounter = 0.0f;
	int32 FrameCounter = 0;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

};
