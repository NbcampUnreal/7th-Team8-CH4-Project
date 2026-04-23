#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "HeistSystemMessageEntry.generated.h"

/**
 * 시스템 메시지 개별 엔트리.
 * BP에서 상속받아 BP_OnShow / BP_OnHide 애니메이션을 구현한다.
 * Duration 만료 시 BP_OnHide 호출 — BP에서 애니메이션 후 RemoveFromParent.
 */
UCLASS()
class HEISTUI_API UHeistSystemMessageEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowMessage(const FText& Text, float Duration);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Message;

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnShow(const FText& Text);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnHide();

private:
	void HandleDurationExpired();

	FTimerHandle DurationTimerHandle;
};
