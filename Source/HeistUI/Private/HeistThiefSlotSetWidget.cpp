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
	if (!IsValid(InPlayerState))
	{
		Cleanup();
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

	// ASC에 바인드하여 상태 변경 감지
	if (IsValid(ThiefImageSlots))
	{
		ThiefImageSlots->BindToASC(InPlayerState->GetHeistAbilitySystemComponent());
	}

	// 현재 상태 태그 확인 및 적용
	if (UHeistAbilitySystemComponent* ASC = InPlayerState->GetHeistAbilitySystemComponent())
	{
		FGameplayTag CurrentStateTag = HeistStateTags::State_Thief_Normal;

		if (ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed))
		{
			CurrentStateTag = HeistStateTags::State_Thief_Cuffed;
		}
		else if (ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured))
		{
			CurrentStateTag = HeistStateTags::State_Thief_Injured;
		}
		else if (ASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Out))
		{
			CurrentStateTag = HeistStateTags::State_Thief_Out;
		}

		ThiefImageSlots->UpdateThiefStateByTag(CurrentStateTag);
	}
}

void UHeistThiefSlotSetWidget::UpdateThiefState(const FGameplayTag& StateTag)
{
	if (!IsValid(ThiefImageSlots))
	{
		return;
	}

	ThiefImageSlots->UpdateThiefStateByTag(StateTag);
}

void UHeistThiefSlotSetWidget::SetPlayerName(const FString& NewPlayerName)
{
	if (!IsValid(Text_PlayerName))
	{
		return;
	}

	Text_PlayerName->SetText(FText::FromString(NewPlayerName));
}

void UHeistThiefSlotSetWidget::Cleanup()
{
	CachedPlayerState = nullptr;

	// 상태 리스너 정리
	if (IsValid(ThiefImageSlots))
	{
		// ThiefSlotWidget에서 자체 정리 처리
	}
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
