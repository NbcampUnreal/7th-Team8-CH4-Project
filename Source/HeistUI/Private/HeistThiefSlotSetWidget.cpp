#include "HeistThiefSlotSetWidget.h"
#include "HeistThiefSlotWidget.h"
#include "Core/HeistPlayerState.h"
#include "Character/HeistTags_State.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"

#include "Components/TextBlock.h"

void UHeistThiefSlotSetWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UHeistThiefSlotSetWidget::NativeDestruct()
{
	Cleanup();
	Super::NativeDestruct();
}

void UHeistThiefSlotSetWidget::Initialize(AHeistPlayerState* InPlayerState)
{
	Cleanup();

	if (!IsValid(InPlayerState))
	{
		return;
	}

	CachedPlayerState = InPlayerState;

	// ASC에 바인드하여 상태 변경 감지
	if (IsValid(ThiefImageSlots))
	{
		ThiefImageSlots->BindToASC(InPlayerState->GetHeistAbilitySystemComponent());
	}

	// 플레이어 이름 설정
	SetPlayerName(InPlayerState->GetPlayerName());
}

void UHeistThiefSlotSetWidget::UpdateThiefState(const FGameplayTag& StateTag)
{
	if (!IsValid(ThiefImageSlots))
	{
		return;
	}

	ThiefImageSlots->UpdateThiefStateByTag(StateTag);
}

void UHeistThiefSlotSetWidget::UpdateThiefStateByName(const FString& StateName)
{
	if (!IsValid(ThiefImageSlots))
	{
		return;
	}

	ThiefImageSlots->UpdateThiefStateByName(StateName);
}

void UHeistThiefSlotSetWidget::SetPlayerName(const FString& NewPlayerName)
{
	CachedPlayerName = NewPlayerName;

	if (!IsValid(Text_PlayerName))
	{
		return;
	}

	Text_PlayerName->SetText(FText::FromString(NewPlayerName));
}

void UHeistThiefSlotSetWidget::Cleanup()
{
	CachedPlayerState = nullptr;
	SetPlayerName(FString());

	// 상태 리스너 정리
	if (IsValid(ThiefImageSlots))
	{
		ThiefImageSlots->BindToASC(nullptr);
	}
}

bool UHeistThiefSlotSetWidget::MatchesPlayer(const AHeistPlayerState* InPlayerState, const FString& InPlayerName) const
{
	if (IsValid(InPlayerState) && CachedPlayerState == InPlayerState)
	{
		return true;
	}

	return !InPlayerName.IsEmpty() && CachedPlayerName.Equals(InPlayerName, ESearchCase::CaseSensitive);
}

void UHeistThiefSlotSetWidget::OnGameplayTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		UpdateThiefState(CallbackTag);
	}
}

void UHeistThiefSlotSetWidget::OnPlayerNameChanged()
{

}
