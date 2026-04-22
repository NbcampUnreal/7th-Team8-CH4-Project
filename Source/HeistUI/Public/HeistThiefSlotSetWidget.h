#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistThiefSlotSetWidget.generated.h"

class AHeistPlayerState;
class UHeistThiefSlotWidget;
class UTextBlock;
/**
 * 
 */
UCLASS()
class HEISTUI_API UHeistThiefSlotSetWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 플레이어 정보를 초기화하고 상태 업데이트 시작.
	 */
	void Initialize(AHeistPlayerState* InPlayerState);

	void UpdateThiefState(const FGameplayTag& StateTag);

	void Cleanup();

	UFUNCTION(BlueprintCallable, Category = "Thief Slot Set")
	void SetPlayerName(const FString& NewPlayerName);

	AHeistPlayerState* GetPlayerState() const { return CachedPlayerState; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHeistThiefSlotWidget> ThiefImageSlots;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerName;

private:
	void OnPlayerNameChanged();
	void OnGameplayTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UPROPERTY()
	TObjectPtr<AHeistPlayerState> CachedPlayerState;

	// 상태 태그 변경 리스너
	FDelegateHandle PlayerNameChangeHandle;
	FDelegateHandle StateTagChangeHandle;

};
