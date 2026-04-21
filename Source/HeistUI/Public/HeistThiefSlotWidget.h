#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistThiefSlotWidget.generated.h"

class UImage;
class UBorder;
struct FGameplayEffectSpec;
struct FActiveGameplayEffectHandle;
class UAbilitySystemComponent;
class UHeistAbilitySystemComponent;

UCLASS()
class HEISTUI_API UHeistThiefSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindToASC(UHeistAbilitySystemComponent* InASC);

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Portrait;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_Injured;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_Handcuffs;

	UPROPERTY(meta = (BindWidget))
	UBorder* Border_StateText;

	UFUNCTION(BlueprintImplementableEvent)
	void OnStatusChanged(const FString& StateName);

	void OnGameplayEffectAdded(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle AGEHandle);

private:
	UPROPERTY()
	TObjectPtr<UHeistAbilitySystemComponent> CachedASC;
};
