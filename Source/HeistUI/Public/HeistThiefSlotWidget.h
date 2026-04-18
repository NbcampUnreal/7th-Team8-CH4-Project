#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistThiefSlotWidget.generated.h"

class UImage;
class UBorder;

UCLASS()
class HEISTUI_API UHeistThiefSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Portrait;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_Injured;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_Handcuffs;

	UPROPERTY(meta = (BindWidget))
	UBorder* Border_StateText;

private:

};
