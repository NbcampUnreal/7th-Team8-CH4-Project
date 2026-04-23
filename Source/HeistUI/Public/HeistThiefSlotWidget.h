#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NativeGameplayTags.h"
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
	void UpdateThiefStateByName(const FString& StateName);

	/**
 * 게임플레이 태그 기반으로 상태 업데이트.
 * 지원하는 상태:
 * - State.Thief.Normal (Idle)
 * - State.Thief.Injured (Injured)
 * - State.Thief.Cuffed (Handcuffed)
 * - State.Thief.Out (Arrested)
 * - 연결 끊김 시 ConnectionLost
 */
	UFUNCTION(BlueprintCallable, Category = "Thief Slot")
	void UpdateThiefStateByTag(const FGameplayTag& StateTag);

	UFUNCTION(BlueprintImplementableEvent, Category = "Thief Slot")
	void OnStatusChanged(const FString& StateName);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void UpdateStateImage(const FString& StateName);
	void ResetStateDisplay();
	void RefreshStateFromASC();
	void UnbindFromASC();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_StateIcon;

	// 상태별 이미지 에셋 (블루프린트에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thief Slot|Images")
	TObjectPtr<UTexture2D> Image_Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thief Slot|Images")
	TObjectPtr<UTexture2D> Image_Injured;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thief Slot|Images")
	TObjectPtr<UTexture2D> Image_Handcuffed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thief Slot|Images")
	TObjectPtr<UTexture2D> Image_Arrested;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thief Slot|Images")
	TObjectPtr<UTexture2D> Image_ConnectionLost;

private:
	//void OnGameplayEffectAdded(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle AGEHandle);

	void OnGameplayTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UPROPERTY()
	TObjectPtr<UHeistAbilitySystemComponent> CachedASC;

	FDelegateHandle StateTagListenerHandle;
	FDelegateHandle InjuredTagListenerHandle;
	FDelegateHandle CuffedTagListenerHandle;
	FDelegateHandle OutTagListenerHandle;
};
