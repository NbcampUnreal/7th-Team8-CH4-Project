#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HeistVoiceSubsystem.generated.h"

class AHeistPlayerController;

UCLASS()
class HEIST_API UHeistVoiceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 트래블 시작 전 voice 전면 종료 (중복 호출 guard 내장)
	void BeginTravelShutdown(UWorld* World);

	// 월드 delegate 수신 — 서브시스템이 직접 바인딩
	void HandleWorldBeginTearDown(UWorld* World);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	// PC 이벤트 수신 — PC가 서브시스템에 알림
	void RequestVoiceRefresh(AHeistPlayerController* PC);
	void OnPawnPossessed(AHeistPlayerController* PC, APawn* NewPawn);
	void OnPlayerStateReady(AHeistPlayerController* PC);
	void TryRecoverVoiceForPlayer(AHeistPlayerController* PC);

	// 판단 레이어 — voice를 켜야 하는지 결정
	void RefreshVoiceState(AHeistPlayerController* PC);
	bool ShouldEnableVoice(AHeistPlayerController* PC) const;

	// 실행 위임 — 최종 Start/Stop은 PC(StartTalking/StopTalking 소유자)가 수행
	void ApplyDesiredVoiceCaptureState(AHeistPlayerController* PC, bool bShouldCapture);

private:
	// 트래블 중 중복 shutdown 방지
	bool bTravelShutdownInProgress = false;

	// PostLoadMap 이후 Pawn/PS 준비가 끝날 때까지 복구를 지연
	bool bPostTravelRecoveryPending = false;

	FDelegateHandle WorldBeginTearDownHandle;
	FDelegateHandle PostLoadMapHandle;
};
