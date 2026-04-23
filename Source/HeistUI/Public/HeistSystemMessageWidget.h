#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistSystemMessageWidget.generated.h"

class UVerticalBox;
class UHeistSystemMessageEntry;
struct FHeistGameNotificationMessage;

/**
 * 시스템 메시지 컨테이너.
 * Message_UI_GameNotification 구독 후 엔트리를 생성해 VBox_Messages에 쌓는다.
 * BP Class Defaults에서 EntryWidgetClass를 WBP_SystemMessageEntry로 지정한다.
 */
UCLASS()
class HEISTUI_API UHeistSystemMessageWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VBox_Messages;

	UPROPERTY(EditDefaultsOnly, Category = "SystemMessage")
	TSubclassOf<UHeistSystemMessageEntry> EntryWidgetClass;

private:
	void HandleGameNotification(FGameplayTag Channel, const FHeistGameNotificationMessage& Msg);

	FHeistMessageListenerHandle MessageHandle;
};
