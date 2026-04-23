#include "HeistSystemMessageWidget.h"

#include "HeistSystemMessageEntry.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

#include "Components/VerticalBox.h"

void UHeistSystemMessageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UHeistMessageSubsystem& MessageSubsystem = UHeistMessageSubsystem::Get(this);
	MessageHandle = MessageSubsystem.RegisterListener<FHeistSystemMessage>(
		HeistMessageTags::Message_UI_SystemMessage,
		[this](FGameplayTag Channel, const FHeistSystemMessage& Msg)
		{
			HandleSystemMessage(Channel, Msg);
		});
}

void UHeistSystemMessageWidget::NativeDestruct()
{
	MessageHandle.Unregister();
	Super::NativeDestruct();
}

void UHeistSystemMessageWidget::HandleSystemMessage(FGameplayTag, const FHeistSystemMessage& Msg)
{
	if (!IsValid(VBox_Messages) || !EntryWidgetClass) return;

	UHeistSystemMessageEntry* Entry = CreateWidget<UHeistSystemMessageEntry>(GetOwningPlayer(), EntryWidgetClass);
	if (!IsValid(Entry)) return;

	VBox_Messages->AddChild(Entry);
	Entry->ShowMessage(Msg.Text, Msg.Duration);
}
