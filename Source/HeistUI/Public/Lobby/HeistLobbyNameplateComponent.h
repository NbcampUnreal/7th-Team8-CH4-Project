#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistLobbyNameplateComponent.generated.h"

class AHeistPlayerState;

/**
 * 로비 캐릭터 머리 위에 닉네임 + 준비 체크 이미지를 표시하는 컴포넌트.
 * 로비 폰 BP에 추가하고, WidgetClass를 WBP_LobbyNameplate로 설정한다.
 * 위치는 BP에서 캐릭터 머리 위로 조정한다.
 */
UCLASS(ClassGroup = "HeistUI", meta = (BlueprintSpawnableComponent))
class HEISTUI_API UHeistLobbyNameplateComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UHeistLobbyNameplateComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TWeakObjectPtr<AHeistPlayerState> BoundPlayerState;

	FDelegateHandle PlayerStateInitializedHandle;
	FHeistMessageListenerHandle ReadyStateHandle;
	FHeistMessageListenerHandle PlayersChangedHandle;

	void InitializeWithPlayerState(AHeistPlayerState* PlayerState);
	void RefreshNameplate();
};
