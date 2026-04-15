#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "HeistBriefingUISubsystem.generated.h"

struct FHeistBriefingContextReadyMessage;
struct FHeistBriefingEndMessage;

/**
 * 브리핑 위젯 생성/소멸을 담당하는 LocalPlayerSubsystem.
 *
 * HeistPlayerController(Heist 모듈)가 UMG를 직접 다루지 않도록
 * 위젯 라이프사이클을 HeistUI 모듈 안으로 격리한다.
 *
 * Message_Briefing_ContextReady 수신 → 위젯 생성 + InitializeForBriefing 호출
 * Message_Briefing_End 수신          → 위젯 제거
 */
UCLASS()
class HEISTUI_API UHeistBriefingUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandleBriefingContextReady(FGameplayTag Channel, const FHeistBriefingContextReadyMessage& Msg);
	void HandleBriefingEnd(FGameplayTag Channel, const FHeistBriefingEndMessage& Msg);

	/**
	 * BP_PlayerController에서 메시지 페이로드로 전달받은 클래스를 사용하므로
	 * 이 서브시스템은 위젯 클래스를 직접 보유하지 않는다.
	 */
	UPROPERTY()
	TObjectPtr<UUserWidget> BriefingWidgetInstance;

	FHeistMessageListenerHandle ContextReadyHandle;
	FHeistMessageListenerHandle EndHandle;
};
