#include "HeistThiefSlotWidget.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Character/HeistTags_State.h"

void UHeistThiefSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UHeistThiefSlotWidget::NativeDestruct()
{
	if (IsValid(CachedASC))
	{
		// 게임플레이 태그 이벤트 해제
		CachedASC->RegisterGameplayTagEvent(HeistStateTags::State_Thief_Normal, EGameplayTagEventType::NewOrRemoved).Remove(StateTagListenerHandle);
		CachedASC->RegisterGameplayTagEvent(HeistStateTags::State_Thief_Injured, EGameplayTagEventType::NewOrRemoved).Remove(InjuredTagListenerHandle);
		CachedASC->RegisterGameplayTagEvent(HeistStateTags::State_Thief_Cuffed, EGameplayTagEventType::NewOrRemoved).Remove(CuffedTagListenerHandle);
	}

	Super::NativeDestruct();
}


void UHeistThiefSlotWidget::BindToASC(UHeistAbilitySystemComponent* InASC)
{
	if (!InASC) return;
	CachedASC = InASC;

	// 상태 태그 변경 리스너 등록
	StateTagListenerHandle = CachedASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Normal,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistThiefSlotWidget::OnGameplayTagChanged);

	InjuredTagListenerHandle = CachedASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Injured,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistThiefSlotWidget::OnGameplayTagChanged);

	CuffedTagListenerHandle = CachedASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Cuffed,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistThiefSlotWidget::OnGameplayTagChanged);

	// 현재 상태 즉시 반영
	if (CachedASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed))
	{
		UpdateThiefStateByTag(HeistStateTags::State_Thief_Cuffed);
	}
	else if (CachedASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured))
	{
		UpdateThiefStateByTag(HeistStateTags::State_Thief_Injured);
	}
	else if (CachedASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Out))
	{
		UpdateThiefStateByTag(HeistStateTags::State_Thief_Out);
	}
	else
	{
		UpdateThiefStateByTag(HeistStateTags::State_Thief_Normal);
	}
}

void UHeistThiefSlotWidget::UpdateThiefStateByTag(const FGameplayTag& StateTag)
{
	FString StateName;

	if (StateTag == HeistStateTags::State_Thief_Injured)
	{
		StateName = TEXT("Injured");
		UpdateStateImage(StateName);
	}
	else if (StateTag == HeistStateTags::State_Thief_Cuffed)
	{
		StateName = TEXT("Handcuffed");
		UpdateStateImage(StateName);
	}
	else if (StateTag == HeistStateTags::State_Thief_Out)
	{
		StateName = TEXT("Arrested");
		UpdateStateImage(StateName);
	}
	else if (StateTag == HeistStateTags::State_Thief_Normal)
	{
		StateName = TEXT("Idle");
		UpdateStateImage(StateName);
	}

	OnStatusChanged(StateName);
}

void UHeistThiefSlotWidget::UpdateStateImage(const FString& StateName)
{
	if (!IsValid(Image_StateIcon))
	{
		return;
	}

	UTexture2D* NewImage = nullptr;

	if (StateName.Equals(TEXT("Normal"), ESearchCase::IgnoreCase))
	{
		NewImage = Image_Idle;
	}
	else if (StateName.Equals(TEXT("Injured"), ESearchCase::IgnoreCase))
	{
		NewImage = Image_Injured;
	}
	else if (StateName.Equals(TEXT("Handcuffed"), ESearchCase::IgnoreCase))
	{
		NewImage = Image_Handcuffed;
	}
	else if (StateName.Equals(TEXT("Out"), ESearchCase::IgnoreCase))
	{
		NewImage = Image_Arrested;
	}
	else if (StateName.Equals(TEXT("ConnectionLost"), ESearchCase::IgnoreCase))
	{
		NewImage = Image_ConnectionLost;
	}

	if (NewImage)
	{
		Image_StateIcon->SetBrushFromTexture(NewImage, true);
	}
}

void UHeistThiefSlotWidget::OnStatusChanged(const FString& StateName)
{
}
//void UHeistThiefSlotWidget::OnGameplayEffectAdded(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle AGEHandle)
//{
//	// 기존 GE 기반 처리는 제거하고 태그 기반 처리로 통합
//}

void UHeistThiefSlotWidget::OnGameplayTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		UpdateThiefStateByTag(CallbackTag);
	}
	else
	{
		// 태그가 제거된 경우 Normal 상태로 복귀
		if (IsValid(CachedASC))
		{
			if (!CachedASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed) &&
				!CachedASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured) &&
				!CachedASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Out))
			{
				UpdateThiefStateByTag(HeistStateTags::State_Thief_Normal);
			}
		}
	}
}
