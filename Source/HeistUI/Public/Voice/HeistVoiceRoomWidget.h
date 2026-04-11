#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistVoiceRoomWidget.generated.h"

class UVerticalBox;
class UHeistVoiceEntryWidget;
class APlayerState;
struct FHeistVoiceTalkingStateMessage;

UCLASS()
class HEISTUI_API UHeistVoiceRoomWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UVerticalBox> PlayerList;

	UPROPERTY(EditDefaultsOnly, Category = "Voice")
	TSubclassOf<UHeistVoiceEntryWidget> EntryWidgetClass;

	UPROPERTY()
	TMap<TObjectPtr<APlayerState>, TObjectPtr<UHeistVoiceEntryWidget>> EntryMap;

	FHeistMessageListenerHandle TalkingStateListenerHandle;

	void InitPlayerList();
	void OnTalkingStateChanged(FGameplayTag Channel, const FHeistVoiceTalkingStateMessage& Message);
};
