#include "HeistThiefSlotSetWidget.h"
#include "HeistThiefSlotWidget.h"
#include "Core/HeistPlayerState.h"

#include "Components/TextBlock.h"

void UHeistThiefSlotSetWidget::UpdateThiefSlots(AHeistPlayerState* PS)
{
	if (PS)
	{
		Text_PlayerNames->SetText(FText::FromString(PS->GetPlayerName()));
		ThiefImageSlots->BindToASC(PS->GetHeistAbilitySystemComponent());
		}
	else
	{
		//TODO_CSH 이탈 처리 추가연결 필요
		//ThiefImageSlots->SetText(FText::FromString(TEXT("이탈됨")));
	}
}
