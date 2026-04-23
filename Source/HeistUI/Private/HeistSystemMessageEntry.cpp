#include "HeistSystemMessageEntry.h"

#include "TimerManager.h"

void UHeistSystemMessageEntry::ShowMessage(const FText& Text, float Duration)
{
	if (IsValid(Text_Message))
	{
		Text_Message->SetText(Text);
	}

	BP_OnShow(Text);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DurationTimerHandle,
			this,
			&ThisClass::HandleDurationExpired,
			Duration,
			false);
	}
}

void UHeistSystemMessageEntry::HandleDurationExpired()
{
	BP_OnHide();
}
