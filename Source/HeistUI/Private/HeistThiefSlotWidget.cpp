#include "HeistThiefSlotWidget.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Character/HeistTags_State.h"

#include "Components/Image.h"
#include "Components/Border.h"
#include "NativeGameplayTags.h"

namespace
{
	const FString ThiefStateIdle(TEXT("Idle"));
	const FString ThiefStateInjured(TEXT("Injured"));
	const FString ThiefStateHandcuffed(TEXT("Handcuffed"));
	const FString ThiefStateArrested(TEXT("Arrested"));
	const FString ThiefStateConnectionLost(TEXT("ConnectionLost"));
}

void UHeistThiefSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UHeistThiefSlotWidget::NativeDestruct()
{
	UnbindFromASC();
	Super::NativeDestruct();
}


void UHeistThiefSlotWidget::BindToASC(UHeistAbilitySystemComponent* InASC)
{
	if (CachedASC == InASC && IsValid(CachedASC))
	{
		RefreshStateFromASC();
		return;
	}

	UnbindFromASC();

	if (!IsValid(InASC))
	{
		ResetStateDisplay();
		return;
	}

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

	OutTagListenerHandle = CachedASC->RegisterGameplayTagEvent(
		HeistStateTags::State_Thief_Out,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UHeistThiefSlotWidget::OnGameplayTagChanged);

	RefreshStateFromASC();
}

void UHeistThiefSlotWidget::UpdateThiefStateByTag(const FGameplayTag& StateTag)
{
	if (StateTag == HeistStateTags::State_Thief_Out)
	{
		UpdateThiefStateByName(ThiefStateArrested);
		return;
	}

	if (StateTag == HeistStateTags::State_Thief_Injured)
	{
		UpdateThiefStateByName(ThiefStateInjured);
		return;
	}

	if (StateTag == HeistStateTags::State_Thief_Cuffed)
	{
		UpdateThiefStateByName(ThiefStateHandcuffed);
		return;
	}

	UpdateThiefStateByName(ThiefStateIdle);
}

void UHeistThiefSlotWidget::UpdateThiefStateByName(const FString& StateName)
{
	UpdateStateImage(StateName);
	OnStatusChanged(StateName);
}

void UHeistThiefSlotWidget::UpdateStateImage(const FString& StateName)
{
	if (!IsValid(Image_StateIcon))
	{
		return;
	}

	UTexture2D* NewImage = nullptr;

	if (StateName.Equals(ThiefStateIdle, ESearchCase::IgnoreCase))
	{
		NewImage = Image_Idle;
	}
	else if (StateName.Equals(ThiefStateInjured, ESearchCase::IgnoreCase))
	{
		NewImage = Image_Injured;
	}
	else if (StateName.Equals(ThiefStateHandcuffed, ESearchCase::IgnoreCase))
	{
		NewImage = Image_Handcuffed;
	}
	else if (StateName.Equals(ThiefStateArrested, ESearchCase::IgnoreCase))
	{
		NewImage = Image_Arrested;
	}
	else if (StateName.Equals(ThiefStateConnectionLost, ESearchCase::IgnoreCase))
	{
		NewImage = Image_ConnectionLost;
	}

	if (NewImage)
	{
		Image_StateIcon->SetBrushFromTexture(NewImage, true);
	}
}

void UHeistThiefSlotWidget::ResetStateDisplay()
{
	if (IsValid(Image_StateIcon))
	{
		Image_StateIcon->SetBrushFromTexture(nullptr, true);
	}

	OnStatusChanged(FString());
}

void UHeistThiefSlotWidget::RefreshStateFromASC()
{
	if (!IsValid(CachedASC))
	{
		ResetStateDisplay();
		return;
	}

	if (CachedASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Out))
	{
		UpdateThiefStateByTag(HeistStateTags::State_Thief_Out);
	}
	else if (CachedASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Cuffed))
	{
		UpdateThiefStateByTag(HeistStateTags::State_Thief_Cuffed);
	}
	else if (CachedASC->HasMatchingGameplayTag(HeistStateTags::State_Thief_Injured))
	{
		UpdateThiefStateByTag(HeistStateTags::State_Thief_Injured);
	}
	else
	{
		UpdateThiefStateByTag(HeistStateTags::State_Thief_Normal);
	}
}

void UHeistThiefSlotWidget::UnbindFromASC()
{
	if (IsValid(CachedASC))
	{
		if (StateTagListenerHandle.IsValid())
		{
			CachedASC->RegisterGameplayTagEvent(
				HeistStateTags::State_Thief_Normal,
				EGameplayTagEventType::NewOrRemoved).Remove(StateTagListenerHandle);
		}

		if (InjuredTagListenerHandle.IsValid())
		{
			CachedASC->RegisterGameplayTagEvent(
				HeistStateTags::State_Thief_Injured,
				EGameplayTagEventType::NewOrRemoved).Remove(InjuredTagListenerHandle);
		}

		if (CuffedTagListenerHandle.IsValid())
		{
			CachedASC->RegisterGameplayTagEvent(
				HeistStateTags::State_Thief_Cuffed,
				EGameplayTagEventType::NewOrRemoved).Remove(CuffedTagListenerHandle);
		}

		if (OutTagListenerHandle.IsValid())
		{
			CachedASC->RegisterGameplayTagEvent(
				HeistStateTags::State_Thief_Out,
				EGameplayTagEventType::NewOrRemoved).Remove(OutTagListenerHandle);
		}
	}

	StateTagListenerHandle.Reset();
	InjuredTagListenerHandle.Reset();
	CuffedTagListenerHandle.Reset();
	OutTagListenerHandle.Reset();
	CachedASC = nullptr;
}

//void UHeistThiefSlotWidget::OnGameplayEffectAdded(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle AGEHandle)
//{
//	// 기존 GE 기반 처리는 제거하고 태그 기반 처리로 통합
//}

void UHeistThiefSlotWidget::OnGameplayTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	RefreshStateFromASC();
}
