#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistThiefSlotSetWidget.generated.h"

class AHeistPlayerState;
class UHeistThiefSlotWidget;
class UTextBlock;
struct FGameplayTag;
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
	void UpdateThiefStateByName(const FString& StateName);

	void Cleanup();

	UFUNCTION(BlueprintCallable, Category = "Thief Slot Set")
	void SetPlayerName(const FString& NewPlayerName);

	AHeistPlayerState* GetPlayerState() const { return CachedPlayerState; }
	const FString& GetCachedPlayerName() const { return CachedPlayerName; }
	bool MatchesPlayer(const AHeistPlayerState* InPlayerState, const FString& InPlayerName) const;

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

	FString CachedPlayerName;

	// 상태 태그 변경 리스너
	FDelegateHandle PlayerNameChangeHandle;
	FDelegateHandle StateTagChangeHandle;

};
