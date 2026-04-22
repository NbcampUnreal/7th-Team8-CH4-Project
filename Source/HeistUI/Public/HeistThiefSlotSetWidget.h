#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistThiefSlotSetWidget.generated.h"

class AHeistPlayerState;
class UHeistThiefSlotWidget;
class UTextBlock;
/**
 * 
 */
UCLASS()
class HEISTUI_API UHeistThiefSlotSetWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateThiefSlots(AHeistPlayerState* PS);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHeistThiefSlotWidget> ThiefImageSlots;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerNames;
};
